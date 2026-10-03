#include "Multiverse.r"

/* Menu Bar Resource */
resource 'MBAR' (128) {
    {
        128,
        129,
        130,
        131
    };
};

/* Apple Menu */
resource 'MENU' (128) {
    128,
    textMenuProc,
    allEnabled,
    enabled,
    apple,
    {
        "About Ftaghn SE...", noIcon, noKey, noMark, plain;
        "-", noIcon, noKey, noMark, plain
    }
};

/* File Menu */
resource 'MENU' (129) {
    129,
    textMenuProc,
    allEnabled,
    enabled,
    "File",
    {
        "New Game", noIcon, "N", noMark, plain;
        "Reset Board", noIcon, "R", noMark, plain;
        "-", noIcon, noKey, noMark, plain;
        "Quit", noIcon, "Q", noMark, plain
    }
};

/* Difficulty Menu */
resource 'MENU' (130) {
    130,
    textMenuProc,
    allEnabled,
    enabled,
    "Difficulty",
    {
        "Simple (Easy)", noIcon, "1", check, plain;
        "Mortal (Normal)", noIcon, "2", noMark, plain;
        "Elder Godlike (Hard)", noIcon, "3", noMark, plain
    }
};

/* Options Menu */
resource 'MENU' (131) {
    131,
    textMenuProc,
    allEnabled,
    enabled,
    "Options",
    {
        "Sound Effects", noIcon, "M", check, plain
    }
};

/* Main Window Resource (500 x 310, centered on 512 x 342) */
resource 'WIND' (128) {
    { 24, 6, 334, 506 },
    documentProc,
    invisible,
    goAway,
    0,
    "FTAGHN - Macintosh SE",
    centerMainScreen
};

/* About Box Alert */
resource 'ALRT' (128) {
    { 60, 60, 260, 452 },
    128,
    {
        OK, visible, silent;
        OK, visible, silent;
        OK, visible, silent;
        OK, visible, silent
    },
    centerMainScreen
};

/* About Box Dialog Item List */
resource 'DITL' (128) {
    {
        { 160, 270, 186, 370 },
        Button {
            enabled,
            "Ph'nglui!"
        },
        { 16, 20, 150, 370 },
        StaticText {
            disabled,
            "FTAGHN: The Call of Ataxx\n"
            "Macintosh SE Edition (Motorola 68000)\n\n"
            "Tailored for 2 MB RAM & System 6/7\n"
            "512 x 342 1-Bit Classic QuickDraw\n\n"
            "\"Ph'nglui mglw'nafh Cthulhu R'lyeh\n"
            " wgah'nagl fhtagn.\""
        }
    }
};

/* MultiFinder / System 7 SIZE Resource */
resource 'SIZE' (-1) {
    reserved,
    acceptSuspendResumeEvents,
    reserved,
    canBackground,
    multiFinderAware,
    backgroundAndForeground,
    dontGetFrontClicks,
    ignoreChildDiedEvents,
    not32BitCompatible,
    isHighLevelEventAware,
    onlyLocalHLEvents,
    notStationeryAware,
    dontUseTextEditServices,
    notDisplayManagerAware,
    reserved,
    reserved,
    384 * 1024,             /* 384 KB Preferred Partition */
    256 * 1024              /* 256 KB Minimum Partition */
};

/* Finder Bundle & File Reference Resources */
resource 'BNDL' (128) {
    'FTAG',
    0,
    {
        'ICN#', { 0, 128 },
        'FREF', { 0, 128 }
    }
};

resource 'FREF' (128) {
    'APPL',
    0,
    ""
};

/* 32x32 1-bit Icon and Mask (Cthulhu Eye & Rune) */
resource 'ICN#' (128) {
    {
        /* Icon Bitmap (32x32) */
        $"0000 0000 0000 0000 007E 0000 01FF C000"
        $"07FF F000 0FFF F800 1FFF FC00 3FFF FE00"
        $"3FFF FE00 7F00 FE00 7C00 3E00 F818 1F00"
        $"F83C 1F00 F07E 0F00 F07E 0F00 F07E 0F00"
        $"F07E 0F00 F83C 1F00 F818 1F00 7C00 3E00"
        $"7E00 7E00 3FFF FE00 3FFF FE00 1FFF FC00"
        $"0FFF F800 07FF F000 01FF C000 007E 0000"
        $"0018 0000 0018 0000 0000 0000 0000 0000",

        /* Icon Mask (32x32) */
        $"0000 0000 0000 0000 007E 0000 01FF C000"
        $"07FF F000 0FFF F800 1FFF FC00 3FFF FE00"
        $"3FFF FE00 7FFF FF00 7FFF FF00 FFFF FF00"
        $"FFFF FF00 FFFF FF00 FFFF FF00 FFFF FF00"
        $"FFFF FF00 FFFF FF00 FFFF FF00 7FFF FF00"
        $"7FFF FF00 3FFF FE00 3FFF FE00 1FFF FC00"
        $"0FFF F800 07FF F000 01FF C000 007E 0000"
        $"0018 0000 0018 0000 0000 0000 0000 0000"
    }
};

/* Include Sound Resources */
#include "sounds.r"
