#define BSM_2K       0
#define BSM_CV       1
#define BSM_4K       2
#define BSM_28SC	 3
#define BSM_F8SC     4
#define BSM_28		 5
#define BSM_F8       6
#define BSM_FE       7
#define BSM_3F       8
#define BSM_E0       9
#define BSM_FA	    10
#define BSM_2A	    11
#define BSM_3A	    12
#define BSM_FANR    13
#define BSM_2ANR    14
#define BSM_3ANR    15
#define BSM_46SC    16
#define BSM_F6SC    17
#define BSM_26SC    18
#define BSM_36SC    19
#define BSM_46      20
#define BSM_F6      21
#define BSM_26      22
#define BSM_36      23
#define BSM_E7      24
#define BSM_E7NR    25
#define BSM_F4SC    26
#define BSM_24SC    27
#define BSM_34SC    28
#define BSM_44SC    29
#define BSM_F4	    30
#define BSM_24		31
#define BSM_34		32
#define BSM_44		33
#define BSM_MB      34
#define FMT_22KHZ_8BIT_MONO 0
#define FMT_44KHZ_8BIT_MONO 1
#define FMT_44KHZ_16BIT_STEREO 2

#define METER_LENGTH 32

#define DISPLAY_NOTHING  0
#define DISPLAY_FILENAME 1
#define DISPLAY_SUMMARY  2
#define DISPLAY_DETAILS  3

#ifdef __linux__
    #define _MAX_FNAME 256
    #define _MAX_PATH  256
    #define _MAX_DIR   256
    #define _MAX_DRIVE 256
    #define _MAX_EXT   256
#else
    #include "find.h"
#endif

typedef unsigned long int   dd;
typedef unsigned short int  dw;
typedef int                 db;


struct parmFlags
{
    int     dflag;
    int     fflag;
    int     kflag;
    int     wflag;
    int     stereoFlag;
    int     mflag;
    int     pflag;
    int     sflag;
    int     vflag;
    char    tflag;
    int     zeroBitLength;
    int     oneBitLength;
    int     control;
    int     createMode;
    int     binFileCount;
    int     bankSwitchMode;
    float   headerSeconds;
    float   clearingSeconds;
    float   hlength;
    char    wavFileName[_MAX_FNAME];
    char    fullPath[_MAX_PATH];

    struct   qnode *binFileQ;
    unsigned int  multi;
    unsigned int  volume;
    unsigned char startHi;
    unsigned char startLo;
};

char bankSwitchList[][5] =
    {"2K",  "CV",   "4K",	"28SC", "F8SC",	"28",	"F8",   "FE",
     "3F",	"E0",   "FA",	"2A",	"3A",	"FANR",	"2ANR",	"3ANR",
	 "46SC","F6SC",	"26SC",	"36SC",	"46",	"F6",	"26",	"36",
     "E7",  "E7NR", "F4SC", "24SC",	"34SC",	"44SC",	"F4",	"24",
	 "34",	"44",	"MB",   ""};

unsigned int controlByteList[] =
    {0xCA,  0xEA,   0xC8,   0xF6,	0xE6,	0xD6,   0xC6,   0xCC,
     0xCE,  0xC1,   0xE0,   0xF0,   0x60,	0xC0,	0xD0,	0x40,
	 0x74,	0xE4,	0xF4,	0x64,	0x54,	0xC4,	0xD4,	0x44,
     0xE3,  0xC3,   0xE2,   0xF2,	0x62,	0x72,	0xC2,	0xD2,
	 0x42,	0x52,	0xC9,   0x00};

int fileSizeList[] =
    {2048,  2048,   4096,   8192,	8192,	8192,   8192,   8192,
     8192,  8192,   12288,  12288,  12288,	12288,	12288,	12288,
	 16384, 16384,	16384,	16384,	16384,	16384,	16384,	16384,
     16384, 16384,  32768,  32768,  32768,	32768,	32768,	32768,
	 32768,	32768,	65536,  0};

int startPageList[] =
    {7,     7,      15,     31,		15,     31,		15,     15,
     31,    31,     15,     31,		47,		15,     31,		47,
	 63,	15,		31,		47,		63,		15,		31,		47,
     63,    63,     15,		31,		47,		63,     15,		31,
     47,	63,		15,     0};

int clearingToneLength[2] = {26, 51};
unsigned char clearingTone[256];

int zeroBitLength,
    oneBitLength;

unsigned char zeroBit[256],
              oneBit[256];

/* speed, khz, bit */
int bitLength[5][2][2] =
{
    {{5,9},{15,21}},
    {{3,7},{ 6,10}},
    {{3,5},{ 5, 8}},
    {{3,5},{ 4, 8}},
    {{3,5},{ 4, 7}}
};

unsigned char header[3][80]=
    {
        {'R','I','F','F',0x80,0x80,0x80,0x80,   /* RIFF TAG */
            'W','A','V','E',                    /* WAV TAG */
                'f','m','t',' ',0x10,0,0,0,     /* FMT TAG */
                    1,0,                        /* format (WAVE_FORMAT_PCM) */
                    1,0,                        /* CHANNELS */
                    0x22,0x56,0,0,              /* SamplesPerSec */
                    0x22,0x56,0,0,              /* BytesPerSec */
                    1,0,                        /* Block align */
                    8,0,                        /* Bits per sample */
                    'd','a','t','a',0,0,0,0,'\0'/* DATA TAG */
        },
        {'R','I','F','F',0x80,0x80,0x80,0x80,   /* RIFF TAG */
            'W','A','V','E',                    /* WAV TAG */
                'f','m','t',' ',0x10,0,0,0,     /* FMT TAG */
                    1,0,                        /* format (WAVE_FORMAT_PCM) */
                    1,0,                        /* CHANNELS */
                    0x44,0xac,0,0,              /* SamplesPerSec */
                    0x44,0xac,0,0,              /* BytesPerSec */
                    1,0,                        /* Block align */
                    8,0,                        /* Bits per sample */
                    'd','a','t','a',0,0,0,0,'\0'/* DATA TAG */
        },
        {'R','I','F','F',0x80,0x80,0x80,0x80,   /* RIFF TAG */
            'W','A','V','E',                    /* WAV TAG */
                'f','m','t',' ',0x10,0,0,0,     /* FMT TAG */
                    1,0,                        /* format (WAVE_FORMAT_PCM) */
                    2,0,                        /* CHANNELS */
                    0x44,0xac,0,0,              /* SamplesPerSec */
                    0x10,0xb1,2,0,              /* BytesPerSec */
                    4,0,                        /* Block align */
                    0x10,0,                     /* Bits per sample */
                    'd','a','t','a',0,0,0,0,'\0'/* DATA TAG */
        }
    };

dd  BS_1[] = {
    0x34ae2945, /* Magicard (CommaVid).bin */
/*  0x30eb4f7a,    Video Life (4K).BIN */
/*  0x9afa761f,    Magicard (Life).bin */
    0x266bd1b6, /* Video Life (CommaVid).bin */
	0xa0899305, /* Video Life (alternative 2K) */
    -1
};

char BS_1_DESC[][25] = {
    "Magicard (CommaVid)",
    "Video Life (CommaVid)",
	"Video Life (CommaVid)",
    -1
};

dd  BS_3[] = {
    0x2843d776, /* Frogger II - Threedeep!.bin */
    0x690ada72, /* Gyruss [b].bin */
    0x525ee7e9, /* Gyruss.bin */
    0x95da4070, /* James Bond 007 [b].bin */
    0x3216c1bb, /* James Bond 007.bin */
    0xae4114d8, /* Montezuma's Revenge.bin */
    0x00e44527, /* Mr. Do!'s Castle.bin */
    0xf723b8a6, /* Popeye.bin */
    0xe44c244e, /* Q-bert's Qubes [a].bin */
    0xb8f2dca6, /* Q-bert's Qubes.bin */
    0xe77f6742, /* Star Wars - Death Star Battle (Parker Bros).bin */
    0xce09fcd4, /* Star Wars - The Arcade Game (Parker Bros).bin */
    0xdd85f0e7, /* Super Cobra [b].bin */
    0x8d372730, /* Super Cobra.bin */
    0xd9088807, /* Tooth Protectors (DSD-Camelot).bin */
    0x7eed7362, /* Tutankham.bin */
    0xc87fc312, /* Popeye_(eks).bin */
    0xef3ec01e, /* Super Cobra_(eks).bin */
    0x84a101d4, /* Star Wars - Death Star Battle_(eks).bin */
    0x2fc06cb0, /* Tutankham_(eks).bin */
    0xab50bf11, /* Star Wars - The Arcade Game (proto).BIN */
    0x549a1b6b, /* Star Wars - The Arcade Game (PAL) */
    0x36910e4d, /* Frogger II - Threedeep! (PAL) */
    0xb8bb2361, /* Gyruss (PAL) */
	0xd3669372, /* Star Wars: Ewok Adventure (PAL) (proto) */
	0x70db5cca, /* Star Wars: Ewok Adventure (T Jentzsch PAL to NTSC Conversion) */
	0x6400d110, /* Lord of the Rings (proto) */
    -1
};

char BS_3_DESC[][50] = {
    "Frogger II - Threedeep!",
    "Gyruss [b]",
    "Gyruss",
    "James Bond 007 [b]",
    "James Bond 007",
    "Montezuma's Revenge",
    "Mr. Do!'s Castle",
    "Popeye",
    "Q-bert's Qubes [a]",
    "Q-bert's Qubes",
    "Star Wars - Death Star Battle (Parker Bros)",
    "Star Wars - The Arcade Game (Parker Bros)",
    "Super Cobra [b]",
    "Super Cobra",
    "Tooth Protectors (DSD-Camelot)",
    "Tutankham",
    "Popeye_(eks)",
    "Super Cobra_(eks)",
    "Star Wars - Death Star Battle_(eks)",
    "Tutankham_(eks)",
    "Star Wars - The Arcade Game (proto)",
    "Star Wars - The Arcade Game (PAL)",
    "Frogger II - Threedeep! (PAL)",
    "Gyruss (PAL)",
	"Star Wars - Ewok Adventure (proto) (PAL)",
	"Star Wars - Ewok Adv. T Jentzsch PAL->NTSC (proto)",
	"Lord of the Rings (proto)",
    ""
};

dd  BS_4[] = {
    0x584f6777, /* Espial [b].bin */
    0x8d70fa42, /* Espial.bin */
    0x8beb03d4, /* Miner 2049er [b1].bin */
    0x33f2856f, /* Miner 2049er [b2].bin */
    0xf859122e, /* Miner 2049er Vol. 2 [b1].bin */
    0x281a1ca1, /* Miner 2049er Vol. 2 [b2].bin */
    0x350c63ba, /* Miner 2049er Vol. 2.bin */
    0x728b941c, /* Miner 2049er.bin */
    0x13bf2da3, /* Polaris [b].bin */
    0x7ce5312e, /* Polaris.bin */
    0x40706361, /* River Patrol (Tigervision).bin */
    0x2c34898f, /* Springer.bin */
    0x09a71583, /* Miner 2049er PAL */
    0xd5645294, /* Miner Volume 2 NTSC */
	0x52630ec9, /* Polaris T. Jentzsch PAL to NTSC conversion*/
    -1
};

char BS_4_DESC[][30] = {
    "Espial [b]",
    "Espial",
    "Miner 2049er [b1]",
    "Miner 2049er [b2]",
    "Miner 2049er Vol. 2 (PAL)[b1]",
    "Miner 2049er Vol. 2 (PAL)[b2]",
    "Miner 2049er Vol. 2 (PAL)",
    "Miner 2049er",
    "Polaris [b] (PAL)",
    "Polaris (PAL)",
    "River Patrol (Tigervision)",
    "Springer",
	"Miner 2049er PAL",
	"Miner 2049er Vol. 2",
	"Polaris (T Jentzsch NTSC->PAL)",
    -1
};

dd  BS_5[] = {
    0x7d23e780, /* Decathlon.bin */
    0xa51c0236, /* Robot Tank.bin */
    0xd8ecf576, /* Decathlon (PAL) */
    0x0e8757b0, /* Robot Tank (PAL) */
    0x94e8df6b, /* Space Shuttle (PAL) */
	0xf3a4363c,	/* Thwocker prototype */
    -1
};

char BS_5_DESC[][25] = {
    "Decathlon",
    "Robot Tank",
    "Decathlon (PAL)",
    "Robot Tank (PAL)",
    "Space Shuttle (PAL)",
	"Thwocker (proto)",
    -1
};

dd	BS_9[] = {
	0x6a31beac,	/* Private Eye (CCE).bin */
	0x3fa749c0,	/* Private Eye [b].bin */
	0x33242242,	/* Private Eye.bin */
	-1
};

char BS_9_DESC[][20] = {
    "Private Eye (CCE)",
    "Private Eye [b]",
    "Private Eye",
    -1
};


dd  BS_6[] = {
    0xa972c32b, /* Dig Dug.bin */
    0x66cdb94b, /* Off the Wall [o].bin */
    0xbd75d92b, /* Off the Wall.bin */
    -1
};

char BS_6_DESC[][20] = {
    "Dig Dug.bin",
    "Off the Wall [o]",
    "Off the Wall",
    -1
};

dd  BS_7[] = {
    0x8eed6b02, /* Bump n Jump [b].bin */
    0xd523e776, /* Bump n Jump.bin */
    0x24c35820, /* Burgertime.bin */
    0x5c161fe4, /* Masters of the Universe - The Power of He-Man.bin */
    -1
};

char BS_7_DESC[][50] = {
    "Bump n Jump [b].bin",
    "Bump n Jump.bin",
    "Burgertime.bin",
    "Masters of the Universe - The Power of He-Man",
    -1
};

/*
dd  BS_8[] = {
        0xbe1047cf,      Fatal Run (PAL).bin
    -1
};
*/

dd crc;         /* holds accumulated CRC */
dd crctab[256];     /* table to help CRC calculation */

#define CRC16_REV 0xA001    /* CRC-16 polynomial reversed */
#define CRC32_REV 0xA0000001    /* CRC-32 polynomial reversed */
#define SQUARE_WAVE 0
#define SINE_WAVE   1
void make16BitStereo(char *sineWave, int *sineWaveLength);
