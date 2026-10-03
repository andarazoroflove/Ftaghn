"""
Motorola 68000 Two-Pass Assembler in Pure Python.
Targeting Macintosh SE / System 7 (Classic 68000 CPU).
"""

import struct
import re

# Mac Toolbox Trap Table
TRAPS = {
    '_INITGRAF': 0xA86E,
    '_INITFONTS': 0xA8FE,
    '_INITWINDOWS': 0xA912,
    '_INITMENUS': 0xA930,
    '_TEINIT': 0xA9CC,
    '_INITDIALOGS': 0xA97B,
    '_INITCURSOR': 0xA850,
    '_SETCURSOR': 0xA851,
    '_WAITNEXTEVENT': 0xA860,
    '_GETNEXTEVENT': 0xA970,
    '_FLUSHEVENTS': 0xA032,
    '_NEWMENU': 0xA931,
    '_APPENDMENU': 0xA933,
    '_INSERTMENU': 0xA935,
    '_DRAWMENUBAR': 0xA937,
    '_HILITEMENU': 0xA938,
    '_MENUBSELECT': 0xA93D,
    '_MENUSELECT': 0xA93D,
    '_NEWWINDOW': 0xA913,
    '_DISPOSEWINDOW': 0xA914,
    '_CLOSEWINDOW': 0xA92D,
    '_SETPORT': 0xA873,
    '_GETPORT': 0xA874,
    '_SELECTWINDOW': 0xA91F,
    '_SHOWWINDOW': 0xA915,
    '_HIDEWINDOW': 0xA916,
    '_TRACKGOAWAY': 0xA91E,
    '_DRAGWINDOW': 0xA925,
    '_FINDWINDOW': 0xA92C,
    '_BEGINUPDATE': 0xA922,
    '_ENDUPDATE': 0xA923,
    '_SYSBEEP': 0xA9C8,
    '_EXITTOSHELL': 0xA9F4,
    '_FRAMERECT': 0xA8A1,
    '_PAINTRECT': 0xA8A2,
    '_ERASERECT': 0xA8A3,
    '_INVERRECT': 0xA8A4,
    '_INVERTRECT': 0xA8A4,
    '_FILLRECT': 0xA8A5,
    '_SETRECT': 0xA8A7,
    '_FRAMEOVAL': 0xA8B7,
    '_PAINTOVAL': 0xA8B8,
    '_ERASEOVAL': 0xA8B9,
    '_INVERTOVAL': 0xA8BA,
    '_FILLOVAL': 0xA8BB,
    '_MOVETO': 0xA893,
    '_LINETO': 0xA891,
    '_DRAWSTRING': 0xA884,
    '_DRAWCHAR': 0xA883,
    '_TEXTFONT': 0xA887,
    '_TEXTSIZE': 0xA88A,
    '_TEXTFACE': 0xA888,
    '_TEXTMODE': 0xA889,
    '_PTINRECT': 0xA8AD,
    '_GETMOUSE': 0xA972,
    '_BUTTON': 0xA974,
    '_GLOBALTOLOCAL': 0xA871,
    '_LOCALTOGLOBAL': 0xA870,
    '_TICKCOUNT': 0xA975,
    '_DELAY': 0xA03B,
    '_MAXAPPLZONE': 0xA063,
    '_MOREMASTERS': 0xA036,
}

CONDITIONS = {
    'RA': 0x0, 'T': 0x0,
    'SR': 0x1, # bsr
    'HI': 0x2, 'LS': 0x3,
    'CC': 0x4, 'HS': 0x4,
    'CS': 0x5, 'LO': 0x5,
    'NE': 0x6, 'EQ': 0x7,
    'VC': 0x8, 'VS': 0x9,
    'PL': 0xA, 'MI': 0xB,
    'GE': 0xC, 'LT': 0xD,
    'GT': 0xE, 'LE': 0xF,
}

class Assembler:
    def __init__(self):
        self.symbols = {}
        self.pc = 0
        self.pass_num = 1

    def eval_expr(self, expr):
        """Evaluate mathematical expression with symbol lookup."""
        expr = expr.strip()
        if not expr:
            return 0
        if expr.startswith("'") and expr.endswith("'") and len(expr) >= 3:
            s = expr[1:-1].encode('latin1')
            val = 0
            for b in s:
                val = (val << 8) | b
            return val
        # Replace hex $FF or 0xFF
        def hex_sub(m):
            return str(int(m.group(1), 16))
        e = re.sub(r'\$([0-9a-fA-F]+)', hex_sub, expr)
        # Replace symbols
        for sym in sorted(self.symbols.keys(), key=len, reverse=True):
            pattern = r'(?<![a-zA-Z0-9_@])' + re.escape(sym) + r'(?![a-zA-Z0-9_@])'
            e = re.sub(pattern, str(self.symbols[sym]), e)
        try:
            return eval(e, {"__builtins__": {}})
        except Exception:
            if self.pass_num == 1:
                return 0 # Forward reference in pass 1
            raise ValueError(f"Unable to evaluate expression: '{expr}' (expanded: '{e}')")

    def parse_ea(self, s):
        """
        Parse 68000 Effective Address.
        Returns (mode, reg, ext_words, is_pc_rel)
        """
        s = s.strip()
        # Data register Dn
        m = re.fullmatch(r'[dD]([0-7])', s)
        if m:
            return (0, int(m.group(1)), [], False)
        # Address register An or SP
        if s.lower() == 'sp':
            return (1, 7, [], False)
        m = re.fullmatch(r'[aA]([0-7])', s)
        if m:
            return (1, int(m.group(1)), [], False)
        # Address register indirect (An)
        if s.lower() == '(sp)':
            return (2, 7, [], False)
        m = re.fullmatch(r'\([aA]([0-7])\)', s)
        if m:
            return (2, int(m.group(1)), [], False)
        # Postincrement (An)+
        if s.lower() == '(sp)+':
            return (3, 7, [], False)
        m = re.fullmatch(r'\([aA]([0-7])\)\+', s)
        if m:
            return (3, int(m.group(1)), [], False)
        # Predecrement -(An)
        if s.lower() == '-(sp)':
            return (4, 7, [], False)
        m = re.fullmatch(r'-\([aA]([0-7])\)', s)
        if m:
            return (4, int(m.group(1)), [], False)

        # Address register indirect with index: d8(An, Xn) or d8(An, Xn.w/l)
        m = re.fullmatch(r'([^()]*)\(([aA][0-7]|[sS][pP])\s*,\s*([dDaA][0-7])(?:\.([wWlL]))?\)', s)
        if m:
            disp_expr = m.group(1).strip() or '0'
            an_str = m.group(2).lower()
            xn_str = m.group(3).lower()
            sz = (m.group(4) or 'w').lower()
            an_reg = 7 if an_str == 'sp' else int(an_str[1])
            da_bit = 1 if xn_str[0] == 'a' else 0
            xn_reg = int(xn_str[1])
            wl_bit = 1 if sz == 'l' else 0
            disp = self.eval_expr(disp_expr) & 0xFF
            ext_word = (da_bit << 15) | (xn_reg << 12) | (wl_bit << 11) | disp
            return (6, an_reg, [ext_word], False)

        # PC relative with index: d8(PC, Xn)
        m = re.fullmatch(r'([^()]*)\([pP][cC]\s*,\s*([dDaA][0-7])(?:\.([wWlL]))?\)', s)
        if m:
            disp_expr = m.group(1).strip() or '0'
            xn_str = m.group(2).lower()
            sz = (m.group(3) or 'w').lower()
            da_bit = 1 if xn_str[0] == 'a' else 0
            xn_reg = int(xn_str[1])
            wl_bit = 1 if sz == 'l' else 0
            disp = self.eval_expr(disp_expr) & 0xFF
            ext_word = (da_bit << 15) | (xn_reg << 12) | (wl_bit << 11) | disp
            return (7, 3, [ext_word], True)

        # Address register indirect with displacement: d(An) or d(sp)
        m = re.fullmatch(r'([^()]+)\(([aA][0-7]|[sS][pP])\)', s)
        if m:
            disp_expr = m.group(1)
            reg_str = m.group(2).lower()
            reg = 7 if reg_str == 'sp' else int(reg_str[1])
            disp = self.eval_expr(disp_expr)
            return (5, reg, [disp & 0xFFFF], False)

        # PC relative with displacement: d(pc)
        m = re.fullmatch(r'([^()]+)\([pP][cC]\)', s)
        if m:
            target = self.eval_expr(m.group(1))
            # PC is instruction address + 2
            disp = target - (self.pc + 2)
            return (7, 2, [disp & 0xFFFF], True)

        # Immediate #imm
        if s.startswith('#'):
            val = self.eval_expr(s[1:])
            return (7, 4, [val], False) # ext words depend on instruction size

        # Absolute long or word
        val = self.eval_expr(s)
        return (7, 1, [(val >> 16) & 0xFFFF, val & 0xFFFF], False)

    def assemble(self, source_text):
        """Assemble source text and return machine code bytes."""
        for pass_num in (1, 2):
            self.pass_num = pass_num
            self.pc = 0
            code = bytearray()
            lines = source_text.splitlines()

            for line_idx, line in enumerate(lines, 1):
                # Strip comments (; or //) outside quotes
                in_quotes = False
                comment_idx = len(line)
                for ci, ch in enumerate(line):
                    if ch == "'":
                        in_quotes = not in_quotes
                    elif not in_quotes:
                        if ch == ';' or (ch == '/' and ci + 1 < len(line) and line[ci + 1] == '/'):
                            comment_idx = ci
                            break
                line = line[:comment_idx].rstrip()
                if not line.strip():
                    continue

                # Handle label: matches label at start of line followed by colon
                m_label = re.match(r'^\s*([a-zA-Z0-9_@.]+):\s*(.*)$', line)
                if m_label:
                    label = m_label.group(1).strip()
                    self.symbols[label] = self.pc
                    line = m_label.group(2).strip()
                    if not line:
                        continue
                else:
                    line = line.strip()

                if not line:
                    continue

                # Split mnemonic and operands
                parts = line.split(None, 1)
                mnemonic = parts[0].upper()
                operands_str = parts[1].strip() if len(parts) > 1 else ""

                # Parse ORG directive
                if mnemonic == 'ORG':
                    self.pc = self.eval_expr(operands_str)
                    continue

                # Parse data directives: DC.B, DC.W, DC.L, DS.B, DS.W, DS.L
                if mnemonic.startswith('DC.') or mnemonic.startswith('DCB.') or mnemonic.startswith('DS.'):
                    code += self.handle_data_directive(mnemonic, operands_str)
                    continue

                # Parse Mac Toolbox A-Traps
                if mnemonic in TRAPS:
                    trap_val = TRAPS[mnemonic]
                    code += struct.pack('>H', trap_val)
                    self.pc += 2
                    continue
                if mnemonic.startswith('_') and mnemonic.upper() in TRAPS:
                    trap_val = TRAPS[mnemonic.upper()]
                    code += struct.pack('>H', trap_val)
                    self.pc += 2
                    continue

                # Parse instructions
                inst_bytes = self.assemble_instruction(mnemonic, operands_str, line_idx)
                code += inst_bytes
                self.pc += len(inst_bytes)

        return bytes(code)

    def handle_data_directive(self, mnemonic, operands_str):
        data = bytearray()
        op, size = mnemonic.split('.')
        size = size.upper()

        if op == 'DS':
            count = self.eval_expr(operands_str)
            mul = {'B': 1, 'W': 2, 'L': 4}[size]
            data += bytes(count * mul)
            self.pc += len(data)
            return data

        if op == 'DCB':
            count_expr, val_expr = [x.strip() for x in operands_str.split(',', 1)]
            count = self.eval_expr(count_expr)
            val = self.eval_expr(val_expr)
            fmt = {'B': '>B', 'W': '>H', 'L': '>I'}[size]
            for _ in range(count):
                data += struct.pack(fmt, val)
            self.pc += len(data)
            return data

        # DC.B, DC.W, DC.L
        tokens = []
        cur = []
        in_quotes = False
        for ch in operands_str:
            if ch == "'":
                in_quotes = not in_quotes
                cur.append(ch)
            elif ch == ',' and not in_quotes:
                tokens.append("".join(cur).strip())
                cur = []
            else:
                cur.append(ch)
        if cur:
            tokens.append("".join(cur).strip())

        for tok in tokens:
            if tok.startswith("'") and tok.endswith("'") and len(tok) >= 2:
                s = tok[1:-1]
                s = s.encode('latin1')
                if size == 'B':
                    data += s
                elif size == 'W':
                    if len(s) % 2 != 0:
                        s += b'\x00'
                    data += s
                elif size == 'L':
                    while len(s) % 4 != 0:
                        s += b'\x00'
                    data += s
            else:
                val = self.eval_expr(tok)
                if size == 'B':
                    data += struct.pack('>B', val & 0xFF)
                elif size == 'W':
                    data += struct.pack('>H', val & 0xFFFF)
                elif size == 'L':
                    data += struct.pack('>I', val & 0xFFFFFFFF)

        self.pc += len(data)
        return data

    def assemble_instruction(self, mnemonic, ops_str, line_idx):
        """Assemble single instruction into bytes."""
        parts = mnemonic.split('.')
        inst = parts[0]
        size = parts[1] if len(parts) > 1 else ('W' if inst not in ('BRA', 'BSR', 'BEQ', 'BNE', 'BLT', 'BLE', 'BGT', 'BGE', 'BCC', 'BCS', 'BPL', 'BMI', 'BHI', 'BLS', 'BVC', 'BVS') else '')

        # Simple zero-operand instructions
        if inst == 'NOP':
            return struct.pack('>H', 0x4E71)
        if inst == 'RTS':
            return struct.pack('>H', 0x4E75)
        if inst == 'ILLEGAL':
            return struct.pack('>H', 0x4AFC)

        # Parse operands
        ops = []
        cur = []
        depth = 0
        in_quotes = False
        for ch in ops_str:
            if ch == "'":
                in_quotes = not in_quotes
                cur.append(ch)
            elif ch == '(' and not in_quotes:
                depth += 1
                cur.append(ch)
            elif ch == ')' and not in_quotes:
                depth -= 1
                cur.append(ch)
            elif ch == ',' and depth == 0 and not in_quotes:
                ops.append("".join(cur).strip())
                cur = []
            else:
                cur.append(ch)
        if cur:
            ops.append("".join(cur).strip())

        # SWAP Dn
        if inst == 'SWAP':
            _, reg, _, _ = self.parse_ea(ops[0])
            return struct.pack('>H', 0x4840 | reg)

        # DBRA / DBF Dn, <label>
        if inst in ('DBRA', 'DBF'):
            _, reg, _, _ = self.parse_ea(ops[0])
            target = self.eval_expr(ops[1])
            disp = target - (self.pc + 2)
            opcode = 0x51C8 | reg
            return struct.pack('>Hh', opcode, disp)

        # Branches: Bcc, BRA, BSR
        if inst in ('BRA', 'BSR') or (inst.startswith('B') and inst[1:] in CONDITIONS):
            cond_code = 0x0 if inst == 'BRA' else (0x1 if inst == 'BSR' else CONDITIONS[inst[1:]])
            target = self.eval_expr(ops[0])
            # Displacement from (PC + 2)
            disp = target - (self.pc + 2)
            if size == 'S' or (-128 <= disp <= 127 and disp != 0 and size != 'W'):
                # 8-bit short branch
                opcode = 0x6000 | (cond_code << 8) | (disp & 0xFF)
                return struct.pack('>H', opcode)
            else:
                # 16-bit word branch
                opcode = 0x6000 | (cond_code << 8)
                return struct.pack('>Hh', opcode, disp)

        # JMP, JSR
        if inst in ('JMP', 'JSR'):
            mode, reg, ext, _ = self.parse_ea(ops[0])
            base_op = 0x4EC0 if inst == 'JMP' else 0x4E80
            opcode = base_op | (mode << 3) | reg
            res = struct.pack('>H', opcode)
            for w in ext:
                res += struct.pack('>H', w & 0xFFFF)
            return res

        # LEA <ea>, An
        if inst == 'LEA':
            src_mode, src_reg, ext, _ = self.parse_ea(ops[0])
            _, dst_reg, _, _ = self.parse_ea(ops[1])
            opcode = 0x41C0 | (dst_reg << 9) | (src_mode << 3) | src_reg
            res = struct.pack('>H', opcode)
            for w in ext:
                res += struct.pack('>H', w & 0xFFFF)
            return res

        # PEA <ea>
        if inst == 'PEA':
            src_mode, src_reg, ext, _ = self.parse_ea(ops[0])
            opcode = 0x4840 | (src_mode << 3) | src_reg
            res = struct.pack('>H', opcode)
            for w in ext:
                res += struct.pack('>H', w & 0xFFFF)
            return res

        # MOVEQ #imm, Dn
        if inst == 'MOVEQ':
            val = self.eval_expr(ops[0].lstrip('#'))
            _, reg, _, _ = self.parse_ea(ops[1])
            opcode = 0x7000 | (reg << 9) | (val & 0xFF)
            return struct.pack('>H', opcode)

        # MOVE / MOVEA
        if inst in ('MOVE', 'MOVEA'):
            size_code = {'B': 1, 'W': 3, 'L': 2}[size]
            src_mode, src_reg, src_ext, _ = self.parse_ea(ops[0])
            dst_mode, dst_reg, dst_ext, _ = self.parse_ea(ops[1])
            
            # Adjust immediate src_ext based on size
            if src_mode == 7 and src_reg == 4:
                val = src_ext[0]
                if size == 'L':
                    src_ext = [(val >> 16) & 0xFFFF, val & 0xFFFF]
                else:
                    src_ext = [val & 0xFFFF]

            opcode = (size_code << 12) | (dst_reg << 9) | (dst_mode << 6) | (src_mode << 3) | src_reg
            res = struct.pack('>H', opcode)
            for w in src_ext:
                res += struct.pack('>H', w & 0xFFFF)
            for w in dst_ext:
                res += struct.pack('>H', w & 0xFFFF)
            return res

        # CLR, TST, NEG, NOT
        if inst in ('CLR', 'TST', 'NEG', 'NOT'):
            size_code = {'B': 0, 'W': 1, 'L': 2}[size]
            mode, reg, ext, _ = self.parse_ea(ops[0])
            base_map = {'CLR': 0x4200, 'TST': 0x4A00, 'NEG': 0x4400, 'NOT': 0x4600}
            opcode = base_map[inst] | (size_code << 6) | (mode << 3) | reg
            res = struct.pack('>H', opcode)
            for w in ext:
                res += struct.pack('>H', w & 0xFFFF)
            return res

        # ADDQ / SUBQ #imm, <ea>
        if inst in ('ADDQ', 'SUBQ'):
            val = self.eval_expr(ops[0].lstrip('#'))
            val_code = 0 if val == 8 else (val & 7)
            size_code = {'B': 0, 'W': 1, 'L': 2}[size]
            mode, reg, ext, _ = self.parse_ea(ops[1])
            base_op = 0x5000 if inst == 'ADDQ' else 0x5100
            opcode = base_op | (val_code << 9) | (size_code << 6) | (mode << 3) | reg
            res = struct.pack('>H', opcode)
            for w in ext:
                res += struct.pack('>H', w & 0xFFFF)
            return res

        # ADD, SUB, AND, OR, CMP
        if inst in ('ADD', 'SUB', 'AND', 'OR', 'CMP'):
            size_code = {'B': 0, 'W': 1, 'L': 2}[size]
            base_map = {'ADD': 0xD000, 'SUB': 0x9000, 'AND': 0xC000, 'OR': 0x8000, 'CMP': 0xB000}
            
            src_mode, src_reg, src_ext, _ = self.parse_ea(ops[0])
            dst_mode, dst_reg, dst_ext, _ = self.parse_ea(ops[1])

            # Immediate source (ADDI, SUBI, CMPI, etc.)
            if src_mode == 7 and src_reg == 4:
                imm_val = src_ext[0]
                imm_ext = [(imm_val >> 16) & 0xFFFF, imm_val & 0xFFFF] if size == 'L' else [imm_val & 0xFFFF]
                base_imm = {'ADD': 0x0600, 'SUB': 0x0400, 'AND': 0x0200, 'OR': 0x0000, 'CMP': 0x0C00}[inst]
                opcode = base_imm | (size_code << 6) | (dst_mode << 3) | dst_reg
                res = struct.pack('>H', opcode)
                for w in imm_ext:
                    res += struct.pack('>H', w & 0xFFFF)
                for w in dst_ext:
                    res += struct.pack('>H', w & 0xFFFF)
                return res

            # <ea>, Dn
            if dst_mode == 0:
                opcode = base_map[inst] | (dst_reg << 9) | (size_code << 6) | (src_mode << 3) | src_reg
                res = struct.pack('>H', opcode)
                for w in src_ext:
                    res += struct.pack('>H', w & 0xFFFF)
                return res
            # Dn, <ea>
            elif src_mode == 0:
                opcode = base_map[inst] | (src_reg << 9) | (1 << 8) | (size_code << 6) | (dst_mode << 3) | dst_reg
                res = struct.pack('>H', opcode)
                for w in dst_ext:
                    res += struct.pack('>H', w & 0xFFFF)
                return res
            # An destination (ADDA, SUBA, CMPA)
            elif dst_mode == 1:
                op_mode = 3 if size == 'W' else 7
                opcode = base_map[inst] | (dst_reg << 9) | (op_mode << 6) | (src_mode << 3) | src_reg
                res = struct.pack('>H', opcode)
                for w in src_ext:
                    res += struct.pack('>H', w & 0xFFFF)
                return res

        # MULU.W <ea>, Dn / DIVU.W <ea>, Dn
        if inst in ('MULU', 'DIVU'):
            src_mode, src_reg, ext, _ = self.parse_ea(ops[0])
            _, dst_reg, _, _ = self.parse_ea(ops[1])
            base_op = 0xC0C0 if inst == 'MULU' else 0x80C0
            opcode = base_op | (dst_reg << 9) | (src_mode << 3) | src_reg
            res = struct.pack('>H', opcode)
            for w in ext:
                res += struct.pack('>H', w & 0xFFFF)
            return res

        # EXT.W Dn, EXT.L Dn
        if inst == 'EXT':
            op_mode = 2 if size == 'W' else 3
            _, reg, _, _ = self.parse_ea(ops[0])
            opcode = 0x4800 | (op_mode << 6) | reg
            return struct.pack('>H', opcode)

        # LINK An, #disp
        if inst == 'LINK':
            _, reg, _, _ = self.parse_ea(ops[0])
            disp = self.eval_expr(ops[1].lstrip('#'))
            return struct.pack('>Hh', 0x4E50 | reg, disp)

        # UNLK An
        if inst == 'UNLK':
            _, reg, _, _ = self.parse_ea(ops[0])
            return struct.pack('>H', 0x4E58 | reg)

        # LSL, LSR, ASL, ASR #count, Dn
        if inst in ('LSL', 'LSR', 'ASL', 'ASR'):
            count = self.eval_expr(ops[0].lstrip('#')) & 7
            if count == 8:
                count = 0
            size_code = {'B': 0, 'W': 1, 'L': 2}[size]
            dir_bit = 1 if inst.startswith('L') or inst.startswith('A') and 'L' in inst else 0
            type_bit = 0 if inst.startswith('A') else 1
            _, reg, _, _ = self.parse_ea(ops[1])
            opcode = 0xE000 | (count << 9) | (dir_bit << 8) | (size_code << 6) | (type_bit << 3) | reg
            return struct.pack('>H', opcode)

        raise ValueError(f"Unknown instruction at line {line_idx}: {mnemonic} {ops_str}")
