; ==============================================================================
; FTAGHN: Cosmic Horror Ataxx for Macintosh SE (68000 / System 7.0+, 2MB RAM)
; Pure Motorola 68000 Assembly Language
; Compatible with Macintosh SE, Classic, Plus, SE/30, and Mini vMac / Basilisk II
; ==============================================================================

    ORG     0

; ------------------------------------------------------------------------------
; Application Entry Point (Segment 1)
; ------------------------------------------------------------------------------
Main:
    ; Expand heap zone to maximum available memory
    _MaxApplZone

    ; Allocate master pointer blocks for smooth memory management
    _MoreMasters
    _MoreMasters

    ; Initialize QuickDraw globals (A5 points to QuickDraw globals boundary)
    PEA     -4(A5)
    _InitGraf

    ; Initialize Toolbox Managers
    _InitFonts
    _InitWindows
    _InitMenus
    _TEInit
    CLR.L   -(SP)               ; resumeProc = nil
    _InitDialogs
    _InitCursor

    ; Flush any pending events in queue
    MOVE.L  #$0000FFFF, D0      ; stopMask=0, everyEvent=0xFFFF
    _FlushEvents

    ; Initialize Game Variables
    BSR     InitGameVariables

    ; Create Menus
    BSR     SetupMenus

    ; Create Main Game Window
    BSR     CreateGameWindow

    ; Initial Full Redraw
    BSR     DrawGameWindow

; ------------------------------------------------------------------------------
; Main Event Loop (Cooperative Multitasking via WaitNextEvent)
; ------------------------------------------------------------------------------
EventLoop:
    ; If AI is pending and game is active, make AI move
    TST.B   gGameOver
    BNE.S   @no_ai
    CMP.B   #2, gCurrPlayer
    BNE.S   @no_ai
    CMP.B   #1, gGameMode       ; Mode 1 = Human vs Human
    BEQ.S   @no_ai

    ; Run AI Move!
    BSR     MakeAIMove
    BSR     DrawGameWindow

@no_ai:
    ; WaitNextEvent(everyEvent, &eventRecord, sleep=6 ticks, nil)
    CLR.W   -(SP)               ; Space for Boolean result
    MOVE.W  #$FFFF, -(SP)       ; everyEvent
    PEA     gEventRecord(PC)    ; VAR theEvent
    MOVE.L  #6, -(SP)           ; 6 ticks = 100ms sleep
    CLR.L   -(SP)               ; mouseRgn = nil
    _WaitNextEvent
    MOVE.W  (SP)+, D0           ; Event occurred?
    TST.W   D0
    BEQ.S   EventLoop           ; Null event, keep looping

    ; Dispatch event based on event.what
    LEA     gEventRecord(PC), A0
    MOVE.W  (A0), D0            ; D0 = event.what

    CMP.W   #1, D0              ; 1 = mouseDown
    BEQ     HandleMouseDown

    CMP.W   #3, D0              ; 3 = keyDown
    BEQ     HandleKeyDown

    CMP.W   #6, D0              ; 6 = updateEvt
    BEQ     HandleUpdateEvt

    BRA     EventLoop

; ------------------------------------------------------------------------------
; Mouse Down Event Handler
; ------------------------------------------------------------------------------
HandleMouseDown:
    LEA     gEventRecord(PC), A0
    MOVE.L  10(A0), D1          ; D1 = event.where (Point: v, h)

    ; FindWindow(where, &whichWindow)
    CLR.W   -(SP)               ; Return part code
    MOVE.L  D1, -(SP)           ; Point where
    PEA     gWhichWindow(PC)    ; VAR WindowPtr
    _FindWindow
    MOVE.W  (SP)+, D0           ; D0 = part code

    CMP.W   #1, D0              ; inMenuBar
    BEQ     HandleMenuClick

    CMP.W   #4, D0              ; inDrag
    BEQ     HandleDragClick

    CMP.W   #6, D0              ; inGoAway
    BEQ     HandleGoAwayClick

    CMP.W   #3, D0              ; inContent
    BEQ     HandleContentClick

    BRA     EventLoop

HandleDragClick:
    MOVE.L  gWhichWindow(PC), -(SP)
    LEA     gEventRecord(PC), A0
    MOVE.L  10(A0), -(SP)       ; startPt
    PEA     gDragRect(PC)       ; boundsRect
    _DragWindow
    BRA     EventLoop

HandleGoAwayClick:
    CLR.W   -(SP)
    MOVE.L  gWhichWindow(PC), -(SP)
    LEA     gEventRecord(PC), A0
    MOVE.L  10(A0), -(SP)       ; startPt
    _TrackGoAway
    MOVE.W  (SP)+, D0
    TST.W   D0
    BEQ     EventLoop
    ; User clicked close box -> Quit Application!
    _ExitToShell

HandleMenuClick:
    LEA     gEventRecord(PC), A0
    CLR.L   -(SP)
    MOVE.L  10(A0), -(SP)       ; startPt
    _MenuSelect
    MOVE.L  (SP)+, D0           ; high=menuID, low=menuItem
    BSR     ExecuteMenuChoice
    BRA     EventLoop

HandleKeyDown:
    LEA     gEventRecord(PC), A0
    MOVE.L  2(A0), D0           ; message (charCode in low byte)
    MOVE.W  14(A0), D1          ; modifiers
    AND.W   #$0100, D1          ; cmdKey bit set?
    BEQ     EventLoop           ; Ignore non-command keys

    AND.W   #$00FF, D0          ; Low byte = ASCII char
    ; Cmd+Q = Quit
    CMP.B   #'q', D0
    BEQ     DoQuit
    CMP.B   #'Q', D0
    BEQ     DoQuit

    ; Cmd+N = New Game
    CMP.B   #'n', D0
    BEQ     DoNewGame
    CMP.B   #'N', D0
    BEQ     DoNewGame

    ; Cmd+R = Restart
    CMP.B   #'r', D0
    BEQ     DoRestart
    CMP.B   #'R', D0
    BEQ     DoRestart

    BRA     EventLoop

DoQuit:
    _ExitToShell

DoNewGame:
    BSR     InitGameVariables
    BSR     DrawGameWindow
    BRA     EventLoop

DoRestart:
    BSR     InitGameVariables
    BSR     DrawGameWindow
    BRA     EventLoop

HandleUpdateEvt:
    MOVE.L  gGameWindow(PC), -(SP)
    _BeginUpdate
    BSR     DrawGameWindow
    MOVE.L  gGameWindow(PC), -(SP)
    _EndUpdate
    BRA     EventLoop

; ------------------------------------------------------------------------------
; Content Click Handler (Board and Buttons)
; ------------------------------------------------------------------------------
HandleContentClick:
    ; Convert global mouse coordinates to window local coordinates
    LEA     gEventRecord(PC), A0
    MOVE.L  10(A0), D1          ; D1 = mouse global Point
    LEA     gMousePt(PC), A1
    MOVE.L  D1, (A1)
    PEA     gMousePt(PC)
    _GlobalToLocal

    ; D1 = local V, D2 = local H
    LEA     gMousePt(PC), A1
    MOVE.W  (A1), D1            ; Mouse V (Y)
    MOVE.W  2(A1), D2           ; Mouse H (X)

    ; Check [New Game] Button: Left=285, Top=225, Right=370, Bottom=245
    CMP.W   #225, D1
    BLT.S   @not_btn_new
    CMP.W   #245, D1
    BGT.S   @not_btn_new
    CMP.W   #285, D2
    BLT.S   @not_btn_new
    CMP.W   #370, D2
    BGT.S   @not_btn_new
    ; Clicked New Game!
    BSR     InitGameVariables
    BSR     DrawGameWindow
    BRA     EventLoop

@not_btn_new:
    ; Check [Pass] Button: Left=385, Top=225, Right=455, Bottom=245
    CMP.W   #225, D1
    BLT.S   @not_btn_pass
    CMP.W   #245, D1
    BGT.S   @not_btn_pass
    CMP.W   #385, D2
    BLT.S   @not_btn_pass
    CMP.W   #455, D2
    BGT.S   @not_btn_pass
    ; Clicked Pass!
    BSR     PassTurn
    BSR     DrawGameWindow
    BRA     EventLoop

@not_btn_pass:
    ; If game over, ignore board clicks
    TST.B   gGameOver
    BNE     EventLoop

    ; If not human's turn in PvAI mode, ignore click
    CMP.B   #2, gCurrPlayer
    BNE.S   @human_turn
    CMP.B   #0, gGameMode       ; PvAI
    BEQ     EventLoop

@human_turn:
    ; Check if click is on 7x7 Board: Left=20, Top=20, Right=258, Bottom=258
    CMP.W   #20, D1
    BLT     EventLoop
    CMP.W   #258, D1
    BGE     EventLoop
    CMP.W   #20, D2
    BLT     EventLoop
    CMP.W   #258, D2
    BGE     EventLoop

    ; Calculate Row: (V - 20) / 34
    SUB.W   #20, D1
    EXT.L   D1
    DIVU.W  #34, D1
    MOVE.W  D1, D3              ; D3 = Row (0..6)

    ; Calculate Col: (H - 20) / 34
    SUB.W   #20, D2
    EXT.L   D2
    DIVU.W  #34, D2
    MOVE.W  D2, D4              ; D4 = Col (0..6)

    ; Process Board Click at (Row D3, Col D4)
    BSR     ProcessCellClick
    BSR     DrawGameWindow
    BRA     EventLoop

; ------------------------------------------------------------------------------
; Process Cell Click at (Row D3, Col D4)
; ------------------------------------------------------------------------------
ProcessCellClick:
    ; Get cell content: index = Row * 7 + Col
    MOVE.W  D3, D0
    MULU.W  #7, D0
    ADD.W   D4, D0
    LEA     gBoard(PC), A0
    MOVE.B  0(A0, D0.W), D5     ; D5 = current cell value

    ; If clicked on current player's piece -> Select it!
    CMP.B   gCurrPlayer, D5
    BNE.S   @check_move_target

    ; Select piece!
    MOVE.B  D3, gSelectedR
    MOVE.B  D4, gSelectedC
    MOVE.W  #1, -(SP)
    _SysBeep
    RTS

@check_move_target:
    ; If no piece is currently selected, nothing to do
    CMP.B   #-1, gSelectedR
    BEQ.S   @done_click

    ; Target must be empty (0)
    TST.B   D5
    BNE.S   @done_click

    ; Calculate Distance: dr = abs(row - selR), dc = abs(col - selC)
    MOVE.B  gSelectedR, D0
    EXT.W   D0
    SUB.W   D3, D0
    BPL.S   @pos_dr
    NEG.W   D0
@pos_dr:
    MOVE.B  gSelectedC, D1
    EXT.W   D1
    SUB.W   D4, D1
    BPL.S   @pos_dc
    NEG.W   D1
@pos_dc:
    ; Max(dr, dc)
    MOVE.W  D0, D2
    CMP.W   D1, D2
    BGE.S   @max_dist
    MOVE.W  D1, D2
@max_dist:
    ; D2 = Distance

    ; Distance 1 = CLONE
    CMP.W   #1, D2
    BEQ.S   @execute_clone

    ; Distance 2 = LEAP
    CMP.W   #2, D2
    BEQ.S   @execute_leap

    ; Distance > 2: Invalid move
    MOVE.W  #2, -(SP)
    _SysBeep
    RTS

@execute_clone:
    ; Place new piece at target (Row D3, Col D4)
    MOVE.W  D3, D0
    MULU.W  #7, D0
    ADD.W   D4, D0
    LEA     gBoard(PC), A0
    MOVE.B  gCurrPlayer, 0(A0, D0.W)

    ; Perform Infection around target
    BSR     InfectNeighbors

    ; Clear selection
    MOVE.B  #-1, gSelectedR
    MOVE.B  #-1, gSelectedC

    ; Update status
    LEA     gMsgClone(PC), A0
    BSR     SetStatusText

    MOVE.W  #3, -(SP)
    _SysBeep

    ; Next turn
    BSR     EndPlayerTurn
    RTS

@execute_leap:
    ; Place piece at target (Row D3, Col D4)
    MOVE.W  D3, D0
    MULU.W  #7, D0
    ADD.W   D4, D0
    LEA     gBoard(PC), A0
    MOVE.B  gCurrPlayer, 0(A0, D0.W)

    ; Clear source piece
    MOVE.B  gSelectedR, D0
    EXT.W   D0
    MULU.W  #7, D0
    MOVE.B  gSelectedC, D1
    EXT.W   D1
    ADD.W   D1, D0
    LEA     gBoard(PC), A0
    CLR.B   0(A0, D0.W)

    ; Perform Infection around target
    BSR     InfectNeighbors

    ; Clear selection
    MOVE.B  #-1, gSelectedR
    MOVE.B  #-1, gSelectedC

    ; Update status
    LEA     gMsgLeap(PC), A0
    BSR     SetStatusText

    MOVE.W  #4, -(SP)
    _SysBeep

    ; Next turn
    BSR     EndPlayerTurn
    RTS

@done_click:
    RTS

; ------------------------------------------------------------------------------
; Infect Opponent Neighbors around (Row D3, Col D4)
; ------------------------------------------------------------------------------
InfectNeighbors:
    ; Opponent = 3 - gCurrPlayer
    MOVE.B  #3, D6
    SUB.B   gCurrPlayer, D6     ; D6 = opponent ID

    ; Check 8 neighbors: dr in [-1, 0, 1], dc in [-1, 0, 1]
    MOVE.W  #-1, D1             ; dr
@loop_dr:
    MOVE.W  #-1, D2             ; dc
@loop_dc:
    ; Skip center (0, 0)
    TST.W   D1
    BNE.S   @check_cell
    TST.W   D2
    BEQ.S   @next_dc

@check_cell:
    MOVE.W  D3, D0
    ADD.W   D1, D0              ; nr = row + dr
    BMI.S   @next_dc
    CMP.W   #7, D0
    BGE.S   @next_dc

    MOVE.W  D4, D5
    ADD.W   D2, D5              ; nc = col + dc
    BMI.S   @next_dc
    CMP.W   #7, D5
    BGE.S   @next_dc

    ; Get cell (nr, nc)
    MULU.W  #7, D0
    ADD.W   D5, D0
    LEA     gBoard(PC), A0
    CMP.B   0(A0, D0.W), D6
    BNE.S   @next_dc

    ; Infect cell!
    MOVE.B  gCurrPlayer, 0(A0, D0.W)

@next_dc:
    ADDQ.W  #1, D2
    CMP.W   #1, D2
    BLE.S   @loop_dc

    ADDQ.W  #1, D1
    CMP.W   #1, D1
    BLE.S   @loop_dr

    RTS

; ------------------------------------------------------------------------------
; End Player Turn & Switch
; ------------------------------------------------------------------------------
EndPlayerTurn:
    BSR     RecalculateScores

    ; Check if game is over (wiped out or board full)
    TST.W   gScore1
    BEQ     CheckGameOver
    TST.W   gScore2
    BEQ     CheckGameOver

    MOVE.W  gScore1, D0
    ADD.W   gScore2, D0
    CMP.W   #49, D0             ; Board completely full?
    BGE     CheckGameOver

    ; Switch Player: 3 - currPlayer
    MOVE.B  #3, D0
    SUB.B   gCurrPlayer, D0
    MOVE.B  D0, gCurrPlayer

    ; Verify if new current player has any valid moves
    BSR     HasValidMoves
    TST.B   D0
    BNE.S   @player_has_moves

    ; No valid moves -> Pass turn automatically!
    LEA     gMsgPass(PC), A0
    BSR     SetStatusText
    MOVE.B  #3, D0
    SUB.B   gCurrPlayer, D0
    MOVE.B  D0, gCurrPlayer

    ; Check if other player ALSO has no moves -> Game over!
    BSR     HasValidMoves
    TST.B   D0
    BEQ     CheckGameOver

@player_has_moves:
    RTS

PassTurn:
    ; Manual pass button
    MOVE.B  #3, D0
    SUB.B   gCurrPlayer, D0
    MOVE.B  D0, gCurrPlayer
    LEA     gMsgPass(PC), A0
    BSR     SetStatusText
    RTS

; ------------------------------------------------------------------------------
; Recalculate Scores
; ------------------------------------------------------------------------------
RecalculateScores:
    CLR.W   gScore1
    CLR.W   gScore2
    LEA     gBoard(PC), A0
    MOVE.W  #48, D0
@count_loop:
    MOVE.B  0(A0, D0.W), D1
    CMP.B   #1, D1
    BNE.S   @not_p1
    ADDQ.W  #1, gScore1
    BRA.S   @next_count
@not_p1:
    CMP.B   #2, D1
    BNE.S   @next_count
    ADDQ.W  #1, gScore2
@next_count:
    DBRA    D0, @count_loop
    RTS

; ------------------------------------------------------------------------------
; Check Game Over
; ------------------------------------------------------------------------------
CheckGameOver:
    BSR     RecalculateScores
    MOVE.B  #1, gGameOver

    MOVE.W  gScore1, D0
    CMP.W   gScore2, D0
    BGT.S   @p1_wins
    BLT.S   @p2_wins

    ; Tie
    CLR.B   gWinner
    LEA     gMsgTie(PC), A0
    BSR     SetStatusText
    RTS

@p1_wins:
    MOVE.B  #1, gWinner
    LEA     gMsgP1Win(PC), A0
    BSR     SetStatusText
    MOVE.W  #10, -(SP)
    _SysBeep
    RTS

@p2_wins:
    MOVE.B  #2, gWinner
    LEA     gMsgP2Win(PC), A0
    BSR     SetStatusText
    MOVE.W  #10, -(SP)
    _SysBeep
    RTS

; ------------------------------------------------------------------------------
; Check If Current Player Has Valid Moves
; Returns D0 = 1 (true) or 0 (false)
; ------------------------------------------------------------------------------
HasValidMoves:
    LEA     gBoard(PC), A0
    ; Search for any piece belonging to current player
    MOVE.W  #0, D1              ; Row
@row_lp:
    MOVE.W  #0, D2              ; Col
@col_lp:
    MOVE.W  D1, D0
    MULU.W  #7, D0
    ADD.W   D2, D0
    MOVE.B  0(A0, D0.W), D3
    CMP.B   gCurrPlayer, D3
    BNE.S   @next_c

    ; Piece found, check all targets at distance 1 or 2
    MOVE.W  #-2, D4             ; dr
@dr_lp:
    MOVE.W  #-2, D5             ; dc
@dc_lp:
    MOVE.W  D1, D6
    ADD.W   D4, D6              ; target row
    BMI.S   @next_t_c
    CMP.W   #7, D6
    BGE.S   @next_t_c

    MOVE.W  D2, D7
    ADD.W   D5, D7              ; target col
    BMI.S   @next_t_c
    CMP.W   #7, D7
    BGE.S   @next_t_c

    ; Target cell must be empty (0)
    MULU.W  #7, D6
    ADD.W   D7, D6
    TST.B   0(A0, D6.W)
    BEQ.S   @found_move

@next_t_c:
    ADDQ.W  #1, D5
    CMP.W   #2, D5
    BLE.S   @dc_lp

    ADDQ.W  #1, D4
    CMP.W   #2, D4
    BLE.S   @dr_lp

@next_c:
    ADDQ.W  #1, D2
    CMP.W   #7, D2
    BLT.S   @col_lp

    ADDQ.W  #1, D1
    CMP.W   #7, D1
    BLT.S   @row_lp

    ; No moves found
    MOVEQ   #0, D0
    RTS

@found_move:
    MOVEQ   #1, D0
    RTS

; ------------------------------------------------------------------------------
; AI Engine (Cosmic Intelligence for Player 2 / Elder Sign)
; ------------------------------------------------------------------------------
MakeAIMove:
    ; Find the best move among all valid moves
    MOVE.W  #-999, gAIBestScore
    MOVE.B  #-1, gAIBestFromR
    MOVE.B  #-1, gAIBestFromC
    MOVE.B  #-1, gAIBestToR
    MOVE.B  #-1, gAIBestToC

    LEA     gBoard(PC), A0
    MOVE.W  #0, D1              ; From Row
@ai_r_lp:
    MOVE.W  #0, D2              ; From Col
@ai_c_lp:
    MOVE.W  D1, D0
    MULU.W  #7, D0
    ADD.W   D2, D0
    MOVE.B  0(A0, D0.W), D3
    CMP.B   #2, D3              ; Must be AI piece (2)
    BNE     @ai_next_from_col

    ; Check targets at distance 1 and 2
    MOVE.W  #-2, D4             ; dr
@ai_dr_lp:
    MOVE.W  #-2, D5             ; dc
@ai_dc_lp:
    MOVE.W  D1, D6
    ADD.W   D4, D6              ; To Row
    BMI.S   @ai_next_to_col
    CMP.W   #7, D6
    BGE.S   @ai_next_to_col

    MOVE.W  D2, D7
    ADD.W   D5, D7              ; To Col
    BMI.S   @ai_next_to_col
    CMP.W   #7, D7
    BGE.S   @ai_next_to_col

    ; Target must be empty
    MOVE.W  D6, D0
    MULU.W  #7, D0
    ADD.W   D7, D0
    TST.B   0(A0, D0.W)
    BNE.S   @ai_next_to_col

    ; Valid move! Calculate score:
    ; Distance: max(abs(dr), abs(dc))
    MOVE.W  D4, D0
    BPL.S   @pdr
    NEG.W   D0
@pdr:
    MOVE.W  D5, D3
    BPL.S   @pdc
    NEG.W   D3
@pdc:
    CMP.W   D3, D0
    BGE.S   @dist_ok
    MOVE.W  D3, D0
@dist_ok:
    ; Base score: Clone (dist 1) = +2 points, Leap (dist 2) = +0 points
    MOVEQ   #0, D3
    CMP.W   #1, D0
    BNE.S   @not_clone_score
    MOVEQ   #2, D3              ; Clone bonus
@not_clone_score:

    ; Count captures: any adjacent cell to (ToR D6, ToC D7) that is Player 1 (1)
    MOVE.W  #-1, A1             ; cap_dr
@cap_dr_lp:
    MOVE.W  #-1, A2             ; cap_dc
@cap_dc_lp:
    MOVE.W  D6, D0
    ADD.W   A1, D0
    BMI.S   @cap_next_dc
    CMP.W   #7, D0
    BGE.S   @cap_next_dc

    MOVE.W  D7, A3
    ADD.W   A2, A3
    BMI.S   @cap_next_dc
    CMP.W   #7, A3
    BGE.S   @cap_next_dc

    MULU.W  #7, D0
    ADD.W   A3, D0
    CMP.B   #1, 0(A0, D0.W)     ; Opponent piece?
    BNE.S   @cap_next_dc
    ADDQ.W  #3, D3              ; +3 points per captured piece!

@cap_next_dc:
    ADDQ.W  #1, A2
    CMP.W   #1, A2
    BLE.S   @cap_dc_lp

    ADDQ.W  #1, A1
    CMP.W   #1, A1
    BLE.S   @cap_dr_lp

    ; Compare with best score
    CMP.W   gAIBestScore, D3
    BLE.S   @ai_next_to_col

    ; New best move!
    MOVE.W  D3, gAIBestScore
    MOVE.B  D1, gAIBestFromR
    MOVE.B  D2, gAIBestFromC
    MOVE.B  D6, gAIBestToR
    MOVE.B  D7, gAIBestToC

@ai_next_to_col:
    ADDQ.W  #1, D5
    CMP.W   #2, D5
    BLE     @ai_dc_lp

    ADDQ.W  #1, D4
    CMP.W   #2, D4
    BLE     @ai_dr_lp

@ai_next_from_col:
    ADDQ.W  #1, D2
    CMP.W   #7, D2
    BLT     @ai_c_lp

    ADDQ.W  #1, D1
    CMP.W   #7, D1
    BLT     @ai_r_lp

    ; Execute best move if found
    CMP.B   #-1, gAIBestFromR
    BEQ.S   @ai_no_moves

    ; Set parameters for move execution
    MOVE.B  gAIBestFromR, gSelectedR
    MOVE.B  gAIBestFromC, gSelectedC
    MOVE.B  gAIBestToR, D3
    EXT.W   D3
    MOVE.B  gAIBestToC, D4
    EXT.W   D4

    ; Calculate distance
    MOVE.B  gSelectedR, D0
    EXT.W   D0
    SUB.W   D3, D0
    BPL.S   @ai_pdr
    NEG.W   D0
@ai_pdr:
    MOVE.B  gSelectedC, D1
    EXT.W   D1
    SUB.W   D4, D1
    BPL.S   @ai_pdc
    NEG.W   D1
@ai_pdc:
    CMP.W   D1, D0
    BGE.S   @ai_max_dist
    MOVE.W  D1, D0
@ai_max_dist:
    CMP.W   #1, D0
    BEQ     @execute_clone
    BRA     @execute_leap

@ai_no_moves:
    ; AI has no moves, pass turn
    BSR     PassTurn
    RTS

; ------------------------------------------------------------------------------
; Menu Execution
; ------------------------------------------------------------------------------
ExecuteMenuChoice:
    ; D0 = (menuID << 16) | menuItem
    SWAP    D0
    MOVE.W  D0, D1              ; D1 = menuID
    SWAP    D0
    MOVE.W  D0, D2              ; D2 = menuItem

    ; Unhighlight menu bar
    CLR.W   -(SP)
    _HiliteMenu

    CMP.W   #128, D1            ; Apple Menu
    BEQ     MenuApple

    CMP.W   #129, D1            ; File Menu
    BEQ     MenuFile

    CMP.W   #130, D1            ; Game Menu
    BEQ     MenuGame

    CMP.W   #131, D1            ; Difficulty Menu
    BEQ     MenuDifficulty

    RTS

MenuApple:
    ; Show About Alert
    MOVE.W  #5, -(SP)
    _SysBeep
    LEA     gMsgAbout(PC), A0
    BSR     SetStatusText
    BSR     DrawGameWindow
    RTS

MenuFile:
    CMP.W   #1, D2              ; 1 = New Game
    BEQ     DoNewGame
    CMP.W   #2, D2              ; 2 = Restart
    BEQ     DoRestart
    CMP.W   #4, D2              ; 4 = Quit
    BEQ     DoQuit
    RTS

MenuGame:
    CMP.W   #1, D2              ; Human vs AI
    BEQ.S   @set_pvai
    CMP.W   #2, D2              ; Human vs Human
    BEQ.S   @set_pvp
    CMP.W   #3, D2              ; AI vs AI
    BEQ.S   @set_aivai
    RTS
@set_pvai:
    CLR.B   gGameMode
    BRA.S   @game_mode_done
@set_pvp:
    MOVE.B  #1, gGameMode
    BRA.S   @game_mode_done
@set_aivai:
    MOVE.B  #2, gGameMode
@game_mode_done:
    BSR     InitGameVariables
    BSR     DrawGameWindow
    RTS

MenuDifficulty:
    SUBQ.W  #1, D2
    MOVE.B  D2, gDifficulty
    RTS

; ------------------------------------------------------------------------------
; Graphics Rendering (1-bit QuickDraw on 512x342 Screen)
; ------------------------------------------------------------------------------
DrawGameWindow:
    ; Set current graphics port to game window
    MOVE.L  gGameWindow(PC), -(SP)
    _SetPort

    ; Erase window background
    PEA     gWinBoundsLocal(PC)
    _EraseRect

    ; Set text characteristics: Chicago font (0), size 12
    MOVE.W  #0, -(SP)           ; 0 = Chicago font
    _TextFont
    MOVE.W  #12, -(SP)
    _TextSize

    ; Draw 7x7 Board Grid
    BSR     DrawBoardGrid

    ; Draw Pieces & Highlights
    BSR     DrawBoardPieces

    ; Draw Right Dashboard Panel
    BSR     DrawDashboard

    RTS

; ------------------------------------------------------------------------------
; Draw 7x7 Board Grid
; ------------------------------------------------------------------------------
DrawBoardGrid:
    ; Outer border: Left=19, Top=19, Right=259, Bottom=259
    PEA     gBoardRectOuter(PC)
    _FrameRect

    ; Draw horizontal grid lines
    MOVE.W  #1, D3
@h_line_lp:
    MOVE.W  D3, D0
    MULU.W  #34, D0
    ADD.W   #20, D0             ; Y = 20 + i * 34

    ; MoveTo(20, Y)
    MOVE.W  #20, -(SP)
    MOVE.W  D0, -(SP)
    _MoveTo

    ; LineTo(258, Y)
    MOVE.W  #258, -(SP)
    MOVE.W  D0, -(SP)
    _LineTo

    ADDQ.W  #1, D3
    CMP.W   #7, D3
    BLT.S   @h_line_lp

    ; Draw vertical grid lines
    MOVE.W  #1, D3
@v_line_lp:
    MOVE.W  D3, D0
    MULU.W  #34, D0
    ADD.W   #20, D0             ; X = 20 + i * 34

    ; MoveTo(X, 20)
    MOVE.W  D0, -(SP)
    MOVE.W  #20, -(SP)
    _MoveTo

    ; LineTo(X, 258)
    MOVE.W  D0, -(SP)
    MOVE.W  #258, -(SP)
    _LineTo

    ADDQ.W  #1, D3
    CMP.W   #7, D3
    BLT.S   @v_line_lp

    RTS

; ------------------------------------------------------------------------------
; Draw Board Pieces and Highlights
; ------------------------------------------------------------------------------
DrawBoardPieces:
    LEA     gBoard(PC), A0
    MOVE.W  #0, D3              ; Row
@r_lp:
    MOVE.W  #0, D4              ; Col
@c_lp:
    ; Compute cell bounding rect:
    ; Top = 20 + Row * 34, Left = 20 + Col * 34
    MOVE.W  D3, D0
    MULU.W  #34, D0
    ADD.W   #20, D0             ; Top
    MOVE.W  D4, D1
    MULU.W  #34, D1
    ADD.W   #20, D1             ; Left

    ; Is this cell currently selected?
    CMP.B   gSelectedR, D3
    BNE.S   @not_selected_cell
    CMP.B   gSelectedC, D4
    BNE.S   @not_selected_cell

    ; Highlight selected cell with inverted/thick frame!
    LEA     gTempRect(PC), A1
    MOVE.W  D0, (A1)            ; Top
    MOVE.W  D1, 2(A1)           ; Left
    ADD.W   #34, D0
    ADD.W   #34, D1
    MOVE.W  D0, 4(A1)           ; Bottom
    MOVE.W  D1, 6(A1)           ; Right
    PEA     gTempRect(PC)
    _InvertRect
    SUB.W   #34, D0
    SUB.W   #34, D1

@not_selected_cell:
    ; Check if cell has a piece: 1 = P1, 2 = P2
    MOVE.W  D3, D2
    MULU.W  #7, D2
    ADD.W   D4, D2
    MOVE.B  0(A0, D2.W), D5

    CMP.B   #1, D5
    BEQ.S   @draw_p1_piece

    CMP.B   #2, D5
    BEQ     @draw_p2_piece

    ; Empty cell: If a piece is selected, check if legal Clone or Leap target
    CMP.B   #-1, gSelectedR
    BEQ     @next_cell

    ; Calculate Distance
    MOVE.B  gSelectedR, D6
    EXT.W   D6
    SUB.W   D3, D6
    BPL.S   @hl_pdr
    NEG.W   D6
@hl_pdr:
    MOVE.B  gSelectedC, D7
    EXT.W   D7
    SUB.W   D4, D7
    BPL.S   @hl_pdc
    NEG.W   D7
@hl_pdc:
    CMP.W   D7, D6
    BGE.S   @hl_max_dist
    MOVE.W  D7, D6
@hl_max_dist:
    ; Distance 1: Draw small Clone square in center (14x14)
    CMP.W   #1, D6
    BNE.S   @check_hl_leap

    LEA     gTempRect(PC), A1
    MOVE.W  D0, D6
    ADD.W   #13, D6
    MOVE.W  D6, (A1)            ; Top = 20 + 13
    MOVE.W  D1, D7
    ADD.W   #13, D7
    MOVE.W  D7, 2(A1)           ; Left
    ADD.W   #8, D6
    ADD.W   #8, D7
    MOVE.W  D6, 4(A1)           ; Bottom
    MOVE.W  D7, 6(A1)           ; Right
    PEA     gTempRect(PC)
    _PaintRect
    BRA     @next_cell

@check_hl_leap:
    ; Distance 2: Draw Leap circle in center (10x10)
    CMP.W   #2, D6
    BNE.S   @next_cell

    LEA     gTempRect(PC), A1
    MOVE.W  D0, D6
    ADD.W   #12, D6
    MOVE.W  D6, (A1)            ; Top
    MOVE.W  D1, D7
    ADD.W   #12, D7
    MOVE.W  D7, 2(A1)           ; Left
    ADD.W   #10, D6
    ADD.W   #10, D7
    MOVE.W  D6, 4(A1)           ; Bottom
    MOVE.W  D7, 6(A1)           ; Right
    PEA     gTempRect(PC)
    _FrameOval
    BRA.S   @next_cell

@draw_p1_piece:
    ; Player 1 (Cult of Cthulhu / Dark Orb):
    ; Solid black circle with diameter 26 in center (inset by 4)
    LEA     gTempRect(PC), A1
    MOVE.W  D0, D6
    ADD.W   #4, D6
    MOVE.W  D6, (A1)            ; Top = Top + 4
    MOVE.W  D1, D7
    ADD.W   #4, D7
    MOVE.W  D7, 2(A1)           ; Left = Left + 4
    ADD.W   #26, D6
    ADD.W   #26, D7
    MOVE.W  D6, 4(A1)           ; Bottom
    MOVE.W  D7, 6(A1)           ; Right
    PEA     gTempRect(PC)
    _PaintOval

    ; White specular highlight dot (Arcane Orb 3D look!) at (Left+8, Top+8, 4x4)
    MOVE.W  (A1), D6
    ADD.W   #5, D6
    MOVE.W  D6, (A1)
    MOVE.W  2(A1), D7
    ADD.W   #5, D7
    MOVE.W  D7, 2(A1)
    ADD.W   #4, D6
    ADD.W   #4, D7
    MOVE.W  D6, 4(A1)
    MOVE.W  D7, 6(A1)
    PEA     gTempRect(PC)
    _EraseOval
    BRA.S   @next_cell

@draw_p2_piece:
    ; Player 2 (Elder Sign / Light Orb):
    ; Thick ring (diameter 26) with inner Elder Cross
    LEA     gTempRect(PC), A1
    MOVE.W  D0, D6
    ADD.W   #4, D6
    MOVE.W  D6, (A1)            ; Top
    MOVE.W  D1, D7
    ADD.W   #4, D7
    MOVE.W  D7, 2(A1)           ; Left
    ADD.W   #26, D6
    ADD.W   #26, D7
    MOVE.W  D6, 4(A1)           ; Bottom
    MOVE.W  D7, 6(A1)           ; Right
    PEA     gTempRect(PC)
    _FrameOval

    ; Second inner concentric ring
    MOVE.W  (A1), D6
    ADD.W   #2, D6
    MOVE.W  D6, (A1)
    MOVE.W  2(A1), D7
    ADD.W   #2, D7
    MOVE.W  D7, 2(A1)
    SUB.W   #2, 4(A1)
    SUB.W   #2, 6(A1)
    PEA     gTempRect(PC)
    _FrameOval

    ; Center Elder Sign cross
    ; Vertical line from Top+7 to Bottom-7
    MOVE.W  D1, D7
    ADD.W   #17, D7             ; Center H
    MOVE.W  D0, D6
    ADD.W   #7, D6              ; Top V
    MOVE.W  D7, -(SP)
    MOVE.W  D6, -(SP)
    _MoveTo
    MOVE.W  D7, -(SP)
    MOVE.W  D0, D6
    ADD.W   #27, D6             ; Bottom V
    MOVE.W  D6, -(SP)
    _LineTo

    ; Horizontal line from Left+7 to Right-7
    MOVE.W  D0, D6
    ADD.W   #17, D6             ; Center V
    MOVE.W  D1, D7
    ADD.W   #7, D7              ; Left H
    MOVE.W  D7, -(SP)
    MOVE.W  D6, -(SP)
    _MoveTo
    MOVE.W  D1, D7
    ADD.W   #27, D7             ; Right H
    MOVE.W  D7, -(SP)
    MOVE.W  D6, -(SP)
    _LineTo

@next_cell:
    ADDQ.W  #1, D4
    CMP.W   #7, D4
    BLT     @c_lp

    ADDQ.W  #1, D3
    CMP.W   #7, D3
    BLT     @r_lp

    RTS

; ------------------------------------------------------------------------------
; Draw Dashboard (Right Side Panel)
; ------------------------------------------------------------------------------
DrawDashboard:
    ; Panel Title
    MOVE.W  #280, -(SP)
    MOVE.W  #35, -(SP)
    _MoveTo
    PEA     gStrTitle(PC)
    _DrawString

    ; Subtitle
    MOVE.W  #280, -(SP)
    MOVE.W  #52, -(SP)
    _MoveTo
    PEA     gStrSubtitle(PC)
    _DrawString

    ; Divider line
    MOVE.W  #280, -(SP)
    MOVE.W  #62, -(SP)
    _MoveTo
    MOVE.W  #470, -(SP)
    MOVE.W  #62, -(SP)
    _LineTo

    ; Turn indicator
    MOVE.W  #280, -(SP)
    MOVE.W  #82, -(SP)
    _MoveTo
    CMP.B   #1, gCurrPlayer
    BEQ.S   @draw_turn_p1

    ; P2 turn
    CMP.B   #1, gGameMode       ; PvP
    BEQ.S   @turn_p2_human
    PEA     gStrTurnAI(PC)
    _DrawString
    BRA.S   @draw_scores
@turn_p2_human:
    PEA     gStrTurnP2(PC)
    _DrawString
    BRA.S   @draw_scores

@draw_turn_p1:
    PEA     gStrTurnP1(PC)
    _DrawString

@draw_scores:
    ; Score 1 (Cthulhu)
    MOVE.W  #280, -(SP)
    MOVE.W  #108, -(SP)
    _MoveTo
    PEA     gStrScoreP1(PC)
    _DrawString

    MOVE.W  gScore1, D0
    BSR     DrawNumber

    ; Score 2 (Elder Sign)
    MOVE.W  #280, -(SP)
    MOVE.W  #128, -(SP)
    _MoveTo
    PEA     gStrScoreP2(PC)
    _DrawString

    MOVE.W  gScore2, D0
    BSR     DrawNumber

    ; Mode display
    MOVE.W  #280, -(SP)
    MOVE.W  #148, -(SP)
    _MoveTo
    CMP.B   #1, gGameMode
    BEQ.S   @mode_pvp
    CMP.B   #2, gGameMode
    BEQ.S   @mode_aivai
    PEA     gStrModePvAI(PC)
    _DrawString
    BRA.S   @draw_status_box
@mode_pvp:
    PEA     gStrModePvP(PC)
    _DrawString
    BRA.S   @draw_status_box
@mode_aivai:
    PEA     gStrModeAIvAI(PC)
    _DrawString

@draw_status_box:
    ; Status Box Frame: Left=278, Top=165, Right=472, Bottom=212
    PEA     gStatusBoxRect(PC)
    _FrameRect

    MOVE.W  #284, -(SP)
    MOVE.W  #185, -(SP)
    _MoveTo
    PEA     gStatusMsg(PC)
    _DrawString

    ; Buttons: [ New Game ] and [ Pass ]
    ; [ New Game ] Button: Left=285, Top=225, Right=370, Bottom=245
    PEA     gBtnNewRect(PC)
    _FrameRect
    MOVE.W  #295, -(SP)
    MOVE.W  #239, -(SP)
    _MoveTo
    PEA     gStrBtnNew(PC)
    _DrawString

    ; [ Pass ] Button: Left=385, Top=225, Right=455, Bottom=245
    PEA     gBtnPassRect(PC)
    _FrameRect
    MOVE.W  #405, -(SP)
    MOVE.W  #239, -(SP)
    _MoveTo
    PEA     gStrBtnPass(PC)
    _DrawString

    RTS

; ------------------------------------------------------------------------------
; Draw Integer Number in D0.W
; ------------------------------------------------------------------------------
DrawNumber:
    ; Convert D0.W (0..99) to 1 or 2 ASCII chars
    AND.W   #$00FF, D0
    DIVU.W  #10, D0
    MOVE.W  D0, D1              ; Tens in D1
    SWAP    D0                  ; Units in D0

    TST.W   D1
    BEQ.S   @draw_units

    ; Draw tens digit
    ADD.B   #'0', D1
    MOVE.W  D1, -(SP)
    _DrawChar

@draw_units:
    ADD.B   #'0', D0
    MOVE.W  D0, -(SP)
    _DrawChar
    RTS

; ------------------------------------------------------------------------------
; Copy Pascal String from A0 to gStatusMsg
; ------------------------------------------------------------------------------
SetStatusText:
    LEA     gStatusMsg(PC), A1
    MOVE.B  (A0)+, D0           ; Length
    MOVE.B  D0, (A1)+
    AND.W   #$00FF, D0
    BEQ.S   @empty_str
    SUBQ.W  #1, D0
@cp_lp:
    MOVE.B  (A0)+, (A1)+
    DBRA    D0, @cp_lp
@empty_str:
    RTS

; ------------------------------------------------------------------------------
; Initialization of Game Variables
; ------------------------------------------------------------------------------
InitGameVariables:
    ; Clear board (49 bytes)
    LEA     gBoard(PC), A0
    MOVE.W  #48, D0
@clr_b_lp:
    CLR.B   0(A0, D0.W)
    DBRA    D0, @clr_b_lp

    ; Place starting pieces in standard Ataxx corner configuration:
    ; Player 1 (Cthulhu): Top-Left (0,0) and Bottom-Right (6,6)
    ; Player 2 (Elder Sign): Top-Right (0,6) and Bottom-Left (6,0)
    MOVE.B  #1, 0(A0)           ; (0, 0) = P1
    MOVE.B  #1, 48(A0)          ; (6, 6) = P1
    MOVE.B  #2, 6(A0)           ; (0, 6) = P2
    MOVE.B  #2, 42(A0)          ; (6, 0) = P2

    MOVE.B  #1, gCurrPlayer     ; Player 1 starts
    MOVE.B  #-1, gSelectedR     ; No selection
    MOVE.B  #-1, gSelectedC
    CLR.B   gGameOver
    CLR.B   gWinner
    MOVE.W  #2, gScore1
    MOVE.W  #2, gScore2

    LEA     gMsgStart(PC), A0
    BSR     SetStatusText
    RTS

; ------------------------------------------------------------------------------
; Setup Menus
; ------------------------------------------------------------------------------
SetupMenus:
    ; Apple Menu (ID 128)
    CLR.L   -(SP)
    MOVE.W  #128, -(SP)
    PEA     gMenuTitleApple(PC)
    _NewMenu
    MOVE.L  (SP), gAppleMenu    ; Save handle
    PEA     gMenuItemAbout(PC)
    _AppendMenu
    MOVE.L  gAppleMenu(PC), -(SP)
    CLR.W   -(SP)               ; beforeID = 0
    _InsertMenu

    ; File Menu (ID 129)
    CLR.L   -(SP)
    MOVE.W  #129, -(SP)
    PEA     gMenuTitleFile(PC)
    _NewMenu
    MOVE.L  (SP), gFileMenu
    PEA     gMenuItemsFile(PC)
    _AppendMenu
    MOVE.L  gFileMenu(PC), -(SP)
    CLR.W   -(SP)
    _InsertMenu

    ; Game Menu (ID 130)
    CLR.L   -(SP)
    MOVE.W  #130, -(SP)
    PEA     gMenuTitleGame(PC)
    _NewMenu
    MOVE.L  (SP), gGameMenu
    PEA     gMenuItemsGame(PC)
    _AppendMenu
    MOVE.L  gGameMenu(PC), -(SP)
    CLR.W   -(SP)
    _InsertMenu

    ; Difficulty Menu (ID 131)
    CLR.L   -(SP)
    MOVE.W  #131, -(SP)
    PEA     gMenuTitleDiff(PC)
    _NewMenu
    MOVE.L  (SP), gDiffMenu
    PEA     gMenuItemsDiff(PC)
    _AppendMenu
    MOVE.L  gDiffMenu(PC), -(SP)
    CLR.W   -(SP)
    _InsertMenu

    ; Draw Menu Bar
    _DrawMenuBar
    RTS

; ------------------------------------------------------------------------------
; Create Main Game Window
; ------------------------------------------------------------------------------
CreateGameWindow:
    ; NewWindow(nil, bounds, title, visible, noGrowDocProc=4, behind=-1, goAway=1, refCon=0)
    CLR.L   -(SP)               ; Result WindowPtr
    CLR.L   -(SP)               ; wStorage = nil (allocate from heap)
    PEA     gWinBoundsGlobal(PC); boundsRect: (38, 16, 328, 496)
    PEA     gWinTitle(PC)       ; title: "Ftaghn - Cosmic Horror Ataxx"
    MOVE.B  #1, -(SP)           ; visible = true
    MOVE.W  #4, -(SP)           ; theProc = noGrowDocProc
    MOVE.L  #-1, -(SP)          ; behind = (WindowPtr)-1
    MOVE.B  #1, -(SP)           ; goAwayFlag = true
    CLR.L   -(SP)               ; refCon = 0
    _NewWindow
    MOVE.L  (SP)+, gGameWindow
    RTS

; ==============================================================================
; DATA SECTION
; ==============================================================================

; --- Rectangles (top, left, bottom, right) ---
gWinBoundsGlobal:
    DC.W    38, 16, 328, 496    ; 480 wide, 290 high
gWinBoundsLocal:
    DC.W    0, 0, 290, 480
gDragRect:
    DC.W    24, 4, 338, 508
gBoardRectOuter:
    DC.W    19, 19, 259, 259
gStatusBoxRect:
    DC.W    165, 278, 212, 472
gBtnNewRect:
    DC.W    225, 285, 245, 370
gBtnPassRect:
    DC.W    225, 385, 245, 455

; --- Window & Menu Handles ---
gGameWindow:
    DC.L    0
gWhichWindow:
    DC.L    0
gAppleMenu:
    DC.L    0
gFileMenu:
    DC.L    0
gGameMenu:
    DC.L    0
gDiffMenu:
    DC.L    0

; --- Event and Mouse Records ---
gEventRecord:
    DS.B    16                  ; what, message, when, where, modifiers
gMousePt:
    DC.W    0, 0
gTempRect:
    DC.W    0, 0, 0, 0

; --- Game State Variables ---
gBoard:
    DS.B    49                  ; 7x7 grid
gCurrPlayer:
    DC.B    1                   ; 1=P1 (Cthulhu), 2=P2 (Elder Sign)
gGameMode:
    DC.B    0                   ; 0=Human vs AI, 1=Human vs Human, 2=AI vs AI
gDifficulty:
    DC.B    1                   ; 0=Easy, 1=Medium, 2=Hard
gSelectedR:
    DC.B    -1                  ; -1 if no piece selected
gSelectedC:
    DC.B    -1
gGameOver:
    DC.B    0
gWinner:
    DC.B    0                   ; 0=Tie, 1=P1, 2=P2
gScore1:
    DC.W    2
gScore2:
    DC.W    2

; --- AI Working Variables ---
gAIBestScore:
    DC.W    0
gAIBestFromR:
    DC.B    0
gAIBestFromC:
    DC.B    0
gAIBestToR:
    DC.B    0
gAIBestToC:
    DC.B    0

; --- Pascal Strings (Length prefix + ASCII) ---
gWinTitle:
    DC.B    28, 'Ftaghn - Cosmic Horror Ataxx'

gMenuTitleApple:
    DC.B    1, 20               ; Apple icon (\x14)
gMenuItemAbout:
    DC.B    21, 'About Ftaghn Ataxx...'

gMenuTitleFile:
    DC.B    4, 'File'
gMenuItemsFile:
    DC.B    33, 'New Game/N;Restart/R;(-;Quit/Q'

gMenuTitleGame:
    DC.B    4, 'Game'
gMenuItemsGame:
    DC.B    39, 'Human vs AI;Human vs Human;AI vs AI'

gMenuTitleDiff:
    DC.B    10, 'Difficulty'
gMenuItemsDiff:
    DC.B    37, 'Mortal (Easy);Elder;Ancient One (Hard)'

gStrTitle:
    DC.B    13, 'FTAGHN: ATAXX'
gStrSubtitle:
    DC.B    19, 'Cosmic Horror Ataxx'

gStrTurnP1:
    DC.B    22, 'Turn: Cult of Cthulhu'
gStrTurnP2:
    DC.B    18, 'Turn: Elder Sign'
gStrTurnAI:
    DC.B    23, 'Turn: Elder AI Thinking'

gStrScoreP1:
    DC.B    18, 'Cult of Cthulhu: '
gStrScoreP2:
    DC.B    18, 'Elder Sign (AI): '

gStrModePvAI:
    DC.B    18, 'Mode: Player vs AI'
gStrModePvP:
    DC.B    19, 'Mode: Player vs Player'
gStrModeAIvAI:
    DC.B    14, 'Mode: AI vs AI'

gStrBtnNew:
    DC.B    8, 'New Game'
gStrBtnPass:
    DC.B    4, 'Pass'

gMsgStart:
    DC.B    24, 'Select an orb to awaken.'
gMsgClone:
    DC.B    23, 'Orb Cloned! Adjacent converted.'
gMsgLeap:
    DC.B    23, 'Orb Leaped! Space warped.'
gMsgPass:
    DC.B    24, 'No moves! Turn passed.'
gMsgTie:
    DC.B    23, 'Game Over: Stalemate!'
gMsgP1Win:
    DC.B    27, 'The Ancient Cult Prevails!'
gMsgP2Win:
    DC.B    25, 'Elder Sign Repels Horror!'
gMsgAbout:
    DC.B    27, 'Ftaghn Ataxx for Mac SE 1.0'

gStatusMsg:
    DS.B    64

