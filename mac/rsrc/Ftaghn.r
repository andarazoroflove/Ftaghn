#include "Multiverse.r"

resource 'MBAR' (128) {
    {
        128,
        129,
        130,
        131
    };
};

resource 'MENU' (128) {
    128,
    textMenuProc,
    allEnabled,
    enabled,
    apple,
    {
        "About Ftaghn...", noIcon, noKey, noMark, plain;
        "-", noIcon, noKey, noMark, plain
    }
};

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

resource 'ALRT' (128) {
    { 80, 100, 240, 450 },
    128,
    {
        OK, visible, silent;
        OK, visible, silent;
        OK, visible, silent;
        OK, visible, silent
    },
    centerMainScreen
};

resource 'DITL' (128) {
    {
        { 116, 230, 142, 330 },
        Button {
            enabled,
            "Ph'nglui!"
        },
        { 16, 20, 104, 330 },
        StaticText {
            disabled,
            "FTAGHN: Cosmic Horror Ataxx\n\n"
            "Carbon Edition for Mac OS 9 & OS X\n"
            "PowerPC G3/G4/G5 & Intel Rosetta\n\n"
            "That is not dead which can eternal lie..."
        }
    }
};

#include "sounds.r"
