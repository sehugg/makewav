/*

   Written by Bob Colbert

   released to the public domain

   The author assumes no responsibility for any damage caused by the use or
   posession of this program.  USE AT YOUR OWN RISK!!!

   Small changes by SvOlli
*/

#include <stdio.h>
#include <string.h>
#include <strings.h>
#include <math.h>
#include <stdlib.h>
#include <unistd.h>
#include "queue.h"
#include "makewav.h"
#ifdef PORTAUDIO
#include "paplay.h"
#else
int palist() { return 0; }
void paplay(const void* start, int used, const char* dev) { }
#endif
#include "streambuffer.h"
#include "version.h"

#define WAV_MODE 0
#define RAW_MODE 1
#define AUD_MODE 2

#define CREATE_IN_RAM 1

FILE  *binFile,
      *wavFile;
unsigned char  buffer[260],
               fileBuffer[0x10000],
               pageList[24],
               emptyPage[256];
float flength,
      wavFlength;

void freadall( void *b, size_t size, FILE *f )
{
   char   *c    = (char*)b;
   size_t pos   = 0;
   size_t dread = 0;

   for( pos = 0; pos < size; pos += size )
   {
      dread = fread( c + pos, 1, size - pos, f );
      if( dread == 0 )
      {
         break;
      }
   }
   if( pos != size )
   {
      fprintf( stderr, "short read: could only get %u bytes out of %u\n",
                       (unsigned int)pos, (unsigned int)size );
      exit( 2 );
   }
}

void processByte( streambuffer_t *buffer, unsigned char pByte, int mode )
{
   int p;

   if( (mode == WAV_MODE) || (mode == AUD_MODE) )
   {
      for(p=0;p<8;p++)
      {
         if (pByte > 127)
         {
            streamadd( buffer, oneBit, oneBitLength );
         }
         else
         {
            streamadd( buffer, zeroBit, zeroBitLength );
         }
         pByte = pByte<<1;
      }
   }
   else
   {
      streamadd( buffer, &pByte, 1 );
   }
}

void getPage(unsigned int page, int display)
{
   if (display)
   {
      printf("%02x\b\b",page);
   }
   fseek(binFile,(long) page * 256,SEEK_SET);
   freadall(buffer,256,binFile);
}

unsigned char setVolume(unsigned int amplitude, int volume)
{
   unsigned int tempval;

   if (amplitude > 128)
   {
      tempval = (unsigned int)floor(((amplitude - 128) * volume)/10 + 128);
   }
   else
   {
      tempval = (unsigned int)floor(128 - ((128 - amplitude) * volume/10));
   }
   return tempval;
}

void make16BitStereo(unsigned char *sineWave, unsigned char *sineWaveLength)
{
   unsigned char oldSineWave[256];
   int i;

   for (i=0; i<*sineWaveLength; i++)
   {
      oldSineWave[i] = sineWave[i];
   }
   for (i=0; i<*sineWaveLength; i++)
   {
      sineWave[i*4]   = 0;
      sineWave[i*4+1] = (oldSineWave[i]+0x80);
      sineWave[i*4+2] = 0;
      sineWave[i*4+3] = (oldSineWave[i]+0x80);
   }
   (*sineWaveLength) *= 4;
}

void setBankSwitchMode(char *pBankSwitch, int *bankSwitchMode)
{
   int i;

   for (i=0; bankSwitchList[i][0] != '\0'; i++)
   {
      if (!strcasecmp(pBankSwitch,bankSwitchList[i]))
      {
         *bankSwitchMode = i;
         break;
      }
   }
   if (controlByteList[i] == 0)
   {
      fprintf(stderr,"Illegal bankswitch type!\n");
      exit(1);
   }
}

void createSineWave(unsigned char *bit, int bitLength, double volume, int wavType)
{
   int i;
   int isOdd;
   int halfBitLength;

   if (volume < 0.10)
   {
      volume = 0.98;
   }
   else
   {
      volume /= 10;
   }

   if (wavType == SQUARE_WAVE)
   {
      halfBitLength = (int)(floor((float)bitLength)/2.0);
      isOdd = (int)(ceil((float)bitLength/2.0) - halfBitLength);
      for (i=0;i<halfBitLength;i++)
      {
         bit[i] = (unsigned char)floor(0xfc * volume);
      }
      if (isOdd)
      {
         bit[i++] = 0x80;
      }
      for (;i<bitLength;i++)
      {
         bit[i] = (unsigned char)floor(0x06 * volume);
      }
   }
   else /* SINE_WAV */
   {
      for (i=0; i<bitLength; i++)
      {
         bit[i] = (unsigned char)floor(((sin((((double)2 * (double)3.1415926535)/(double)bitLength)*(double)i)*volume + 1)*128));
      }
   }
}

int processParms( int argc, char *argv[], struct parmFlags *parms )
{
#if 0
   parms->control = 0;
   parms->sflag = 0;
   parms->kflag = 1;
   parms->wflag = SQUARE_WAVE;
   parms->stereoFlag = 0;
   parms->fflag = 1;
   parms->zeroBitLength = 0;
   parms->oneBitLength = 0;
   parms->aflag = 0;
   parms->dflag = DISPLAY_FILENAME;
   parms->mflag = 0;
   parms->pflag = 0;
   parms->vflag = 0;
   parms->volume = 0;
   parms->tflag = 'c';
   parms->multi = 0;
   parms->headerSeconds = (float)1.0;
   parms->clearingSeconds = (float)0.1;
   parms->hlength = 0;
   parms->bankSwitchMode = -1;
   parms->createMode = WAV_MODE;
   parms->audioDevName = 0;
   strcpy(parms->fullPath,"");
   strcpy(parms->wavFileName,"");
#else
   memset( parms, 0, sizeof(*parms) );
   parms->kflag = 1;
   parms->wflag = SQUARE_WAVE;
   parms->fflag = -1;
   parms->dflag = DISPLAY_FILENAME;
   parms->tflag = 'c';
   parms->headerSeconds = (float)1.0;
   parms->clearingSeconds = (float)0.1;
   parms->bankSwitchMode = -1;
   parms->createMode = WAV_MODE;
#endif

   while (--argc > 0 && (*++argv)[0] == '-')
   {
      char flag,
            cflag;

      cflag = 0;
      flag = *++argv[0];
      switch (flag)
      {
      case 'a':
         parms->aflag = 1;
         if( strlen((char*)(*argv)+1) > 0 )
         {
            parms->audioDevName = (char*)(*argv)+1;
         }
         break;
      case 'l':
         exit( palist() );
      case 'd':
         if (sscanf(*argv,"d%1d",&parms->dflag) == EOF)
         {
            fprintf(stderr,"Illegal detail level!\n");
            exit(1);
         }
         break;
      case 'p':
         parms->pflag = 1;
         break;
      case 'f':
         if (sscanf(*argv,"f%1d",&parms->fflag) == EOF)
         {
            fprintf(stderr,"Illegal .wav speed!\n");
            exit(1);
         }
         if (parms->fflag > 4)
         {
            parms->fflag = 4;
         }
         break;
      case 'b':
         if (!cflag)
         {
            setBankSwitchMode(*argv+1, &parms->bankSwitchMode);
            break;
         }
      case 'c':
         cflag = 1;
         if (strlen(*argv) != 3)
         {
            fprintf(stderr,"Illegal control byte value!\n");
            exit(1);
         }
         if (sscanf(*argv,"c%2x",&parms->control) == EOF)
         {
            fprintf(stderr,"Illegal control byte value!\n");
            exit(1);
         }
         printf("Control byte set to %02x\n",parms->control);
         break;
      case 'h':
         if (sscanf(*argv,"h%f",&parms->headerSeconds) == EOF)
         {
            fprintf(stderr,"Illegal header length!\n");
            exit(1);
         }
         if (parms->headerSeconds > 10 || parms->headerSeconds <=0)
         {
            printf("Bad header length!\n");
            exit(1);
         }
         break;
      case '0':
         if (sscanf(*argv,"0%d",&parms->zeroBitLength) == EOF)
         {
            fprintf(stderr,"Zero-bit length must be provided for 0 flag!\n");
            exit(1);
         }
         if (parms->zeroBitLength > 80 || parms->zeroBitLength <1)
         {
            fprintf(stderr,"Bad zero-bit length!\n");
            exit(1);
         }
         break;
      case '1':
         if (sscanf(*argv,"1%d",&parms->oneBitLength) == EOF)
         {
            printf("One-bit length must be provided for 0 flag!\n");
            exit(1);
         }
         if (parms->oneBitLength > 80 || parms->oneBitLength <1)
         {
            printf("Bad one-bit length!\n");
            exit(1);
         }
         break;
      case 'i':
         if (sscanf(*argv,"i%f",&parms->clearingSeconds) == EOF)
         {
            fprintf(stderr,"Illegal clearing tone length!\n");
            exit(1);
         }
         if (parms->clearingSeconds > 10 || parms->clearingSeconds <= 0.1)
         {
            fprintf(stderr,"Bad clearing tone length!\n");
            exit(1);
         }
         break;
      case 's':
         if (strlen(*argv) !=5)
         {
            fprintf(stderr,"Illegal start address!\n");
            exit(1);
         }
         {
            unsigned int start;
            if (sscanf(*argv,"s%04x",&start) == EOF)
            {
               fprintf(stderr,"Illegal start address!\n");
               exit(1);
            }
            parms->startHi = (start >> 8) & 0xff;
            parms->startLo = start & 0xff;
         }
         parms->sflag = 1;
         break;
      case 't':
      {
         int noValue = 0;

         if (sscanf(*argv,"t%c",&parms->tflag) == EOF)
         {
            noValue = 1;
         }
         if (noValue || (parms->tflag != 's' && parms->tflag != 'c'))
         {
            fprintf(stderr,"Type flag 't' must be 's' for Supercharger or 'c' for Cuttle Cart\n");
            exit(1);
         }
      }
         break;
      case 'w':
         if (sscanf(*argv,"w%1d",&parms->wflag) == EOF)
         {
            fprintf(stderr,"Illegal wave form type\n");
            exit(1);
         }
         if (parms->wflag > 1)
         {
            fprintf(stderr,"Illegal wave form type\n");
            exit(1);
         }
         break;
      case 'k':
         parms->kflag = 0;
         if (sscanf(*argv,"k%1d",&parms->kflag) == EOF)
         {
            fprintf(stderr,"Illegal .wav format!\n");
            exit(1);
         }
         if (parms->kflag < 0)
         {
            fprintf(stderr,"Illegal .wav format!\n");
            exit(1);
         }
         if (parms->kflag >= 2)
         {
            parms->kflag = 1;
            parms->stereoFlag = 1;
         }
         break;
      case 'o':
         if (sscanf(*argv,"o%s", &parms->wavFileName[0]) == EOF)
         {
            fprintf(stderr,"Output filename required for o flag\n");
            exit(1);
         }
         break;
      case 'm':
         if (strlen(*argv) != 3)
         {
            fprintf(stderr,"Illegal multi-byte value!\n");
            exit(1);
         }
         if (sscanf(*argv,"m%2x",&parms->multi) == EOF)
         {
            fprintf(stderr,"Illegal multi-byte value!\n");
            exit(1);
         }
         printf("Multi-load byte is $%02x\n",parms->multi);
         parms->mflag = 1;
         break;
      case 'r':
         parms->createMode = RAW_MODE;
         break;
      case 'v':
         if ((strlen(*argv) == 1) || (strlen(*argv)>3))
         {
            fprintf(stderr,"Illegal volume value, must be 1-10\n");
            exit(1);
         }
         parms->vflag = 1;
         if (sscanf(*argv,"v%2i",&parms->volume) == EOF)
         {
            fprintf(stderr,"Illegal volume value, must be 1-10\n");
            exit(1);
         }
         if ((((int)parms->volume < 1)) || ((int)parms->volume > 10))
         {
            fprintf(stderr,"Illegal volume value, must be 1-10\n");
            exit(1);
         }
         break;
      default :
         fprintf(stderr,"Unknown flag %c\n",flag);
         exit(1);
         break;
      }
   }
   parms->binFileCount = 0;
   parms->binFileQ = NULL;
   while (argc)
   {
      char tempFileName[256],
            extension[256],
            *tempPtr;

      strcpy(tempFileName, argv[0]);
      strtok(argv[0],".");
      tempPtr = strtok(NULL,"\0");
      if (tempPtr != 0)
         strcpy(extension,tempPtr);
      parms->binFileQ = addq(parms->binFileQ, tempFileName, &parms->binFileCount);
      ++argv;
      argc--;
   }

   if (parms->aflag)
   {
      /* direct audio audioput is hardcoded to 44kHz mono */
      parms->createMode = AUD_MODE;
      parms->stereoFlag = 0;
      parms->kflag      = 1;
   }

   if( parms->fflag < 0 )
   {
      parms->fflag = ( parms->tflag == 'c' ) ? 1 : 0;
   }

   if (!parms->binFileCount)
   {
      printf( "Makewav " MAKEWAV_VERSION "\n"
              " Written By: Bob Colbert (rcolbert1@home.com)\n"
              " Direct Audio Support By: Sven Oliver Moll (svolli@svolli.de)\n"
              " Usage: makewav [flags] singleLd.bin [multiLd1[+multiLd2...+multiLdn]]\n"
              " Flags:\n"
              "     -a<NAME> Output though audio device <NAME> (<NAME> is optional)\n"
              "              (<NAME> allows trailing \"*\" as joker, first match is taken)\n"
              "     -b<MODE> Set bankswitch mode:\n"
              "              2K,CV,4K,F8SC,28SC,F8,28,FE,3F,E0,FA,2A,3A,FANR,2ANR,\n"
              "              3ANR,F6SC,26SC,36SC,46SC,F6,26,36,46,E7,E7NR,F4SC,\n"
              "              24SC,34SC,44SC,F4,24,34,44,MB\n"
              "                Where Fx and FxSC are Fx started in the first bank,\n"
              "                and y8,y8SC,y6,y6SC,y4,y4SC are F8,F8SC,F6,F6SC,F4,\n"
              "                F4SC resectively started in bank y.\n"
              "     -cxx     Set control byte (2 digit hex, overrides control byte\n"
              "              determined by -b)\n"
              "     -d#      Diagnostics detail level (0 - 3)\n"
              "     -f#      Speed of load (0 - 4, Default=1 for CC, 0 for SC)\n"
              "     -h#      Set header tone length in seconds (default = 1.0)\n"
              "     -i#      Set clearing tone length in seconds (default = 0.1)\n"
              "     -k       Frequency: 0=22khz, 1(Default)=44khz, 2=16-bit stereo 44khz\n"
              "     -l       list available audio device names\n"
              "     -mxx     Set multiload byte (2 digit hex)\n"
              "     -o       Output File (Default = <singleLd.wav> or <multiLd1.wav>)\n"
              "     -p       Force \"blank pages\" to be transferred\n"
              "     -r       Create raw data file instead of a .wav file\n"
              "     -sxxxx   Set start address (4 digit hex)\n"
              "     -t<TYPE> c=Cuttle Cart (Default) or s=Supercharger\n"
              "     -v#      Set volume level of .wav file (0.10-10 Default=0.98)\n"
              "     -w#      Wave form type: 0 (Default) for square or 1 for sine\n"
              "     -0#      Length of zero bit sine wave in bytes (overrides -f)\n"
              "     -1#      Length of one bit sine wave in bytes (overrides -f)\n" );
      exit(0);
   }

   return 0;
}

/*
** used for generating the CRC lookup table
*/

dd crcrevhware(dd data, dd genpoly, dd accum)
{
   int i;
   data <<= 1;
   for (i=8;i>0;i--)
   {
      data >>= 1;
      if ((data ^ accum) & 1)
      {
         accum = (accum >> 1) ^ genpoly;
      }
      else
      {
         accum >>= 1;
      }
   }
   return(accum);
}


/*
** init the CRC lookup table
*/

void init_crc(void)
{
   int i;
   for (i=0;i<256;i++)
   {
      crctab[i] = crcrevhware(i,CRC32_REV,0);
   }
}


/*
** update CRC
*/

void ucrc(unsigned char data)
{
   crc = (crc >> 8) ^ crctab[(crc ^ data) & 0xff];
}

db Lookup(dd *table)
{
   //  dd t;
   int i = 0;

   while(table[i] != -1)
   {
      if (table[i] == crc)
         return i;
      i++;
   }
   return -1;
   /*
    while(1)
    {
        t = *table++;
        if (t == -1)  return(0);
        if (t == crc)
        {
            if (table == BS_3)
                printf("- %s",BS_3_DESC[i]);
            return(1);
        }
        i++;
    }
    */
}

void clearMeter(int meterMax)
{
   int i;
   printf("%*s",meterMax+2," ");
   for (i=0; i<meterMax+2; i++)
   {
      printf("\b");
   }

   //  printf("%*s",meterMax+2,"\b");
}

void drawMeter(int current, int max, int meterMax)
{
   int i,
         meterLength;

   meterLength = (int)((float)current/(float)(max-1)*meterMax);
   printf("[");
   for (i=0; i<meterMax; i++)
   {
      if (i<meterLength)
      {
         printf("*");
      }
      else
      {
         printf("-");
      }
   }
   printf("]");
   for (i=0; i<meterMax+2; i++)
   {
      printf("\b");
   }
}

int main( int argc, char *argv[] )
{
   int i = 0,
         j = 0,
         k = 0,
         filecount = 0,
         gamePageCount = 0,
         scflag = 0,
         file_page_count = 0,
         currentBinFile = 0,
         bytesPerSecond = 0;

   unsigned char
         controlByte = 0,
         in_byte = 0,
         multiByte = 0,
         speedLow = 0,
         speedHi = 0;

   unsigned int
         length_lword[4],
         sum = 0,
         page = 0,
         init_bank = 0;

   char pg_bank_byte = 0;

   long fsize = 0;

   struct parmFlags parms;
   char full_path_buffer[256];

   streambuffer_t streambuffer;
   streaminit( &streambuffer );

   processParms(argc, argv, &parms);

   if (parms.zeroBitLength)
   {
      zeroBitLength = parms.zeroBitLength;
   }
   else
   {
      zeroBitLength = bitLength[parms.fflag][parms.kflag][0];
   }

   if (parms.oneBitLength)
   {
      oneBitLength = parms.oneBitLength;
   }
   else
   {
      oneBitLength = bitLength[parms.fflag][parms.kflag][1];
   }

   createSineWave(clearingTone, clearingToneLength[parms.kflag],parms.volume, SINE_WAVE);
   createSineWave(zeroBit,zeroBitLength, parms.volume, parms.wflag);
   createSineWave(oneBit,oneBitLength, parms.volume, parms.wflag);

   /* 0x55 sends 4 "01" bit combinations so the formula to calculate the header length is */
   /* floor((samples per second/bytes per "01" sample)/4) */
   bytesPerSecond = (int)(((parms.kflag == 0)?22050:44100)/(oneBitLength+zeroBitLength))/4;
   parms.hlength = (float)floor(parms.headerSeconds * bytesPerSecond);

   if (parms.stereoFlag)
   {
      make16BitStereo(clearingTone, &(clearingToneLength[1]));
      make16BitStereo(zeroBit, &zeroBitLength);
      make16BitStereo(oneBit, &oneBitLength);
   }
   scflag = 0;
   currentBinFile = 0;
   if (parms.binFileCount > 4)
   {
      char answer;

      fprintf(stderr,"Warning! %d .wav files will be created, Continue? (Y/N)", parms.binFileCount);
      while( scanf("%c",&answer) < 1 )
      {
         if (answer != 'y' && answer != 'Y')
         {
            exit(1);
         }
      }
   }

   if (parms.dflag >= DISPLAY_FILENAME)
   {
      printf("Converting %d file%s for use with the %s:\n",
              parms.binFileCount, (parms.binFileCount > 1)?"s":"",
              (parms.tflag == 's')?"Supercharger":"Cuttle Cart");
      printf("  Using %s %skhz ",(parms.stereoFlag)?"16-bit stereo":"8-bit mono",(parms.kflag == 0)?"22":"44");
      printf( parms.aflag ? "direct audio": ".wav format");
      printf(" with a %0.2f second header tone\n",parms.headerSeconds);
      printf("  (zero bit/one bit) lengths = (%d/%d)", zeroBitLength/((parms.stereoFlag)?4:1), oneBitLength/((parms.stereoFlag)?4:1));
      printf(" implemented with %s wave forms\n",(parms.wflag == 0)?"square":"sine");
   }

   while (parms.binFileQ != NULL)
   {
      int  bankSwitchMode = -1;
      char *sourceFileName;
      char parmEntry[256];
      char baseFileName[256];
      int  multiLoad;

      if (parms.dflag >= DISPLAY_FILENAME)
      {
         printf("%s -> ",parms.binFileQ->fileName);
      }
      filecount = 0;
      flength = 0;
      wavFlength = 0;
      strcpy(parmEntry, parms.binFileQ->fileName);
      strcpy(baseFileName, strtok(parms.binFileQ->fileName,".+"));
      if (!strcmp(parms.wavFileName,""))
      {
         snprintf(&parms.wavFileName[0], sizeof(parms.wavFileName)-1, "%s.wav", baseFileName);
      }
      sourceFileName = strtok(parmEntry,"+");
      wavFile = 0;
      multiLoad = 0;
      while (sourceFileName != NULL)
      {
         page = 0;
         if ((binFile = fopen(sourceFileName,"rb")) == NULL)
         {
            fprintf(stderr,"ERROR: unable to open .bin file %s\n",sourceFileName);
            exit(1);
         }

         fseek(binFile,0,SEEK_END);
         fsize = ftell(binFile);
         fseek(binFile,0,SEEK_SET);
         freadall(fileBuffer,fsize,binFile);
         if (parms.tflag == 's' && fsize != 8448 && fsize != 6144 && fsize != 2048 && fsize != 4096 && fsize != 32767)
         {
            fprintf(stderr,"Invalid file size for Supercharger, skipping\n");
            break;
         }
         if (!wavFile && (parms.createMode != AUD_MODE) )
         {
            strcpy(full_path_buffer,  parms.wavFileName);

            if ((wavFile = fopen(full_path_buffer,"wb")) == NULL)
            {
               fprintf(stderr,"ERROR: unable to create wavFile %s\n",full_path_buffer);
               exit(1);
            }
            if (parms.dflag >= DISPLAY_FILENAME)
            {
               printf("%s",full_path_buffer);
            }
         }

         /* Write .wav header */
         if (parms.createMode == WAV_MODE && !multiLoad)
         {
            for(k=0;k<44;k++)
            {
               fputc(header[parms.kflag + parms.stereoFlag][k],wavFile);
            }
         }

         multiByte = 0;
         init_bank = 0;
         /* For original supercharger only */
         if (parms.tflag == 's')
         {
            if (fsize == 2048)
            {
               init_bank = 2;
            }
            else if (fsize == 4096)
            {
               init_bank = 1;
            }
         }
         scflag = 0;
         /* Check for 32k Supercharger games */
         if (fsize == 0x7FFF)
         {
            scflag = 1;
         }

         if (fsize == 6144 || fsize == 8448 || scflag)
         {
            /* This is the control byte, it determines the bankswitching mode of the SC

                Bits                Function
                ------              --------
                D7-D5               Write Pulse Delay (Set to 0)
                D4-D2               RAM/ROM Configuration

                                    Value     $f000     $f800
                                    -----     -----     -----
                                     000        3        ROM
                                     001        1        ROM
                                     010        3         1
                                     011        1         3
                                     100        3        ROM
                                     101        2        ROM
                                     110        3         2
                                     111        2         3
                */
            if (!parms.control)
            {
               controlByte = 0x0d; /* %00001101 */
            }
            speedLow  = 0x42;
            speedHi = 0x02;
            file_page_count = 24;
            if (fsize >= 8448)
            {
               getPage(32,0);
               if (!parms.sflag)
               {
                  parms.startLo=buffer[0];
                  parms.startHi=buffer[1];
               }
               //                  parms.sflag = 1;
               if (!parms.control)
               {
                  controlByte = buffer[2];
               }
               file_page_count = buffer[3];
               scflag = 1;
               for (i=0;i<0x18;i++)
               {
                  pageList[i] = buffer[0x10+i];
               }
               multiByte = buffer[5];
            }
         }
         else
         {
            int i;
            /* Find file size in list */
            for (i=0; fileSizeList[i] != 0; i++)
            {
               if (fsize == fileSizeList[i])
               {
                  /* if the bank-switch mode wasn't set in the parms, use default */
                  if (parms.bankSwitchMode < 0)
                  {
                     bankSwitchMode = i;
                  }
                  else if (fileSizeList[parms.bankSwitchMode] != fsize)
                  {
                     fprintf(stderr,"Bankswitch scheme %s is not legal for a %dK ROM\n",
                             bankSwitchList[parms.bankSwitchMode],
                           (int)(fsize/1024));
                     exit(1);
                  }
                  else /* bank-switch mode was set in the parms */
                  {
                     bankSwitchMode = parms.bankSwitchMode;
                  }
                  break;
               }
            }
            if (fileSizeList[i] == 0)
            {
               fprintf(stderr, "ERROR!  Source .bin file must be 2048, 4096, 6144, 8192, 8448, 12228, 16384, 32768, or 65536 bytes\n");
               exit(1);
            }
            file_page_count = fileSizeList[bankSwitchMode] / 256;
         }
         gamePageCount = 0;
         for(i=0;i<file_page_count;i++)
         {
            getPage(i,0);
            emptyPage[i] = 1;
            for (j=0;j<255;j++)
            {
               if (buffer[j] != buffer[j+1])
               {
                  emptyPage[i] = 0;
               }
            }
            if (!emptyPage[i])
            {
               gamePageCount++;
            }
         }

         speedHi = gamePageCount/21 + 1;
         speedLow = gamePageCount*256/21 - (speedHi-1)*256;

         /*Write low frequency clearing tone*/
         if (!multiLoad)
         {
            float sineCount;

            sineCount = (float)floor(parms.clearingSeconds * (((parms.kflag == 0)?22050:44100)/(clearingToneLength[parms.kflag])));
            if (parms.stereoFlag)
            {
               sineCount /= 2;
            }
            for(k=0;k<sineCount;k++)
            {
               streamadd( &streambuffer, &clearingToneLength[parms.kflag], 1 );
            }
         }
         /* Write game header tone, just a series of alternating zero and one bits */
         for(k=1;k<(parms.hlength);k++)
         {
            processByte( &streambuffer, 0x55, parms.createMode );
         }

         /* Two zero bits in a row indicate the beginning of the data */
         processByte( &streambuffer, 0x54, parms.createMode );

         if (parms.bankSwitchMode < 0 )
         {
            int i;

            crc = 0 ;
            init_crc();
            for (i=0; i<fsize; i++)
               ucrc(fileBuffer[i]) ;

            switch(fsize)
            {
            case 0x800:
               if ((i=Lookup(BS_1)) >= 0)
               {
                  bankSwitchMode = BSM_CV;    /* CommaVid */
                  if (parms.dflag >= DISPLAY_FILENAME)
                     printf("- %s",BS_1_DESC[i]);
               }
               break;
            case 0x2000:
               for (i=0; i<2; i++)
               {
                  for (j=0; j<256; j++)
                  {
                     if (fileBuffer[0] != fileBuffer[i * 0x1000 + j])
                     {
                        bankSwitchMode = BSM_28;
                     }
                  }
               }

               if ((i=Lookup(BS_3)) >= 0)
               {
                  bankSwitchMode = BSM_E0;   /* Parker Brothers */
                  if (parms.dflag >= DISPLAY_FILENAME)
                  {
                     printf("- %s",BS_3_DESC[i]);
                  }
               }
               if ((i=Lookup(BS_4)) >= 0)
               {
                  bankSwitchMode = BSM_3F; /* Tigervision */
                  if (parms.dflag >= DISPLAY_FILENAME)
                  {
                     printf("- %s",BS_4_DESC[i]);
                  }
               }
               if ((i=Lookup(BS_5)) >= 0)
               {
                  bankSwitchMode = BSM_FE;   /* Activision 8K flat model */
                  if (parms.dflag >= DISPLAY_FILENAME)
                  {
                     printf("- %s",BS_5_DESC[i]);
                  }
               }
               if ((i=Lookup(BS_9)) >= 0)
               {
                  bankSwitchMode = BSM_F8;   /* Atari F8 Start in Bank 1 */
                  if (parms.dflag >= DISPLAY_FILENAME)
                  {
                     printf("- %s",BS_9_DESC[i]);
                  }
               }
               break;
            case 0x4000:
               for (i=0; i<4; i++)
               {
                  for (j=0; j<256; j++)
                  {
                     if (fileBuffer[0] != fileBuffer[i * 0x1000 + j])
                     {
                        bankSwitchMode = BSM_46 ;
                     }
                  }
               }
               if ((i=Lookup(BS_6)) >= 0)
               {
                  bankSwitchMode = BSM_46SC;      /* 16K Superchip that can't be recognized automatically */
                  if (parms.dflag >= DISPLAY_FILENAME)
                  {
                     printf("- %s",BS_6_DESC[i]);
                  }
               }
               if ((i=Lookup(BS_7)) >= 0)
               {
                  bankSwitchMode = BSM_E7;        /* M Network 16K */
                  if (parms.dflag >= DISPLAY_FILENAME)
                  {
                     printf("- %s",BS_7_DESC[i]);
                  }
               }
               break;
            case 0x8000:
               for (i=0; i<8; i++)
               {
                  for (j=0; j<256; j++)
                  {
                     if (fileBuffer[0] != fileBuffer[i * 0x1000 + j])
                     {
                        bankSwitchMode = BSM_F4;
                     }
                  }
               }
               break;
            case 0x10000:
               if (bankSwitchMode == BSM_MB)
               {
                  parms.pflag = 1;
                  printf("Forcing blank pages to be transferred for\n");
                  printf("bankswitch mode MB\n");
               }
               break;
            }
         }

         if (parms.pflag)
         {
            gamePageCount = file_page_count;
         }

         if (parms.control)
         {
            controlByte = parms.control;
         }
         else if (parms.tflag == 's')
         {
            if (!scflag)
               controlByte = 0x1d;
            if (fsize == 2048)
            {
               getPage(startPageList[bankSwitchMode],0);
               if (buffer[0xfd] < 0xf8)
               {
                  controlByte = 0x09;
               }
            }
         }
         else if (!scflag)
         {
            controlByte = controlByteList[bankSwitchMode];
         }


         /* The first two bytes of data indicate the beginning address of the code */

         if ((!parms.sflag) && !scflag)
         {
            getPage(startPageList[bankSwitchMode],0);
            parms.startLo = buffer[0xfc];
            parms.startHi = buffer[0xfd];
         }

         processByte( &streambuffer, parms.startLo, parms.createMode );
         processByte( &streambuffer, parms.startHi, parms.createMode );

         processByte( &streambuffer, controlByte, parms.createMode );

         /* Number of pages to load */

         processByte( &streambuffer, (unsigned char)(gamePageCount), parms.createMode );

         /* Game header checksum -- first 8 bytes must add up to 0x55 */

         if( parms.mflag )
         {
            multiByte = parms.multi;
         }

         processByte( &streambuffer, (unsigned char)(0x55 - parms.startLo - parms.startHi - multiByte - controlByte - gamePageCount - speedLow - speedHi), parms.createMode );
         processByte( &streambuffer, multiByte, parms.createMode );
         processByte( &streambuffer, speedLow, parms.createMode );
         processByte( &streambuffer, speedHi, parms.createMode );
         if( parms.dflag >= DISPLAY_FILENAME )
         {
            printf("\n");
         }
         if( parms.dflag >= DISPLAY_SUMMARY )
         {
            printf("  Bankswitch type   = ");
            if (scflag)
            {
               printf("Supercharger\n");
            }
            else
            {
               printf("%s\n",bankSwitchList[bankSwitchMode]);
            }
            printf("  File size         = %-5ld",fsize);
            printf("  Start address     = $%02x%02x\n",parms.startHi,parms.startLo);
            printf("  Multi-Load byte   = $%2.2x    Page count        = $%2.2x\n",(unsigned char) multiByte,(unsigned char) gamePageCount);
            printf("  Control-byte      = $%2.2x    Blank pages       = $%2.2x\n",(unsigned char) controlByte,(unsigned char) (file_page_count - gamePageCount));
         }
         while ((int)page < file_page_count)
         {
            int j;

            /* Get appropriate page */

            if (!emptyPage[page] || parms.pflag)
            {
               getPage(page,0);
               if (parms.dflag >= DISPLAY_FILENAME && parms.dflag < DISPLAY_DETAILS)
                  drawMeter(page, gamePageCount, METER_LENGTH);
            }

            /* If we just got the page, put the page header which consists of two bytes. The first byte */
            /* is a counter that begins at zero for the first page and is incremented by 4 for each      */
            /* subsequent page.  If the value is greater than 0x1f, then 0x1f is subtracted from it.     */

            if (scflag)
            {
               pg_bank_byte = pageList[page];
            }
            else
            {
               pg_bank_byte = (page % 8) * 4 + init_bank + (int)floor((page-(int)floor(page/32)*32)/8) + (int)floor(page/32) * 32;
            }
            sum = 0;

            /* Get the sum of all 256 bytes of the current page */

            for(j=0;j<256;j++)
            {
               sum += (char) buffer[j];
            }

            /* The second byte of the page header is 0x55 - the first byte - the sum of the 256 bytes of  */
            /* program data.                                                                              */

            in_byte = (char) (0x55 - pg_bank_byte - sum);

            /* Put the program data */
            if (!emptyPage[page] || parms.pflag)
            {
               if (parms.dflag >= DISPLAY_DETAILS)
               {
                  printf(" - bank %2.2d, page %2.2d, page&bank byte %2.2x, checksum %2.2x\n",(int)(pg_bank_byte&0x3) + (int)(pg_bank_byte&0xE0)/8,
                         (int)((pg_bank_byte & 0x1c)/4),(unsigned char)pg_bank_byte,
                         (unsigned char)in_byte);
               }

               processByte( &streambuffer, pg_bank_byte,parms.createMode);
               processByte( &streambuffer, in_byte,parms.createMode);
               for(j=0;j<256;j++)
               {
                  processByte( &streambuffer, buffer[j],parms.createMode);
               }
            }

            /* Done with this page, increment page counter */

            page++;
            /*  printf("\nPage is = %d",page); */
         }

         /* We are done, put a tone at the end of the game, unnecessary, but a nice touch */
         fclose(binFile);
         filecount++;

         sourceFileName = strtok(NULL,"+");
         if (sourceFileName != NULL)
         {
            for(k=1;k<parms.headerSeconds * bytesPerSecond * 2;k++)
            {
               processByte( &streambuffer, 0x55,parms.createMode);
            }
            multiLoad = 1;
         }
         else
            for(k=1;k<bytesPerSecond/2;k++)
            {
               processByte( &streambuffer, 0x55,parms.createMode);
            }
      } /* End of strtok While */

      if ((parms.createMode == WAV_MODE) && wavFile)
      {

         /* Determine the file length for the WAV chunk of the .wav file */
         flength = streambuffer.used;
         wavFlength = flength + 36;

         /* Write the file length in hi-byte/lo-byte, hi-word/lo-word format to the .wav file */
         fseek(wavFile,0x28,SEEK_SET);
         length_lword[3] = 0;
         length_lword[2] = 0;
         length_lword[1] = 0;
         if (flength > 16777216)
         {
            length_lword[3] = (int)floor(flength / 16777216);
            flength -= length_lword[3] * 16777216;
         }
         if (flength > 65535)
         {
            length_lword[2] = (int)floor(flength / 65536);
            flength -= length_lword[2] * 65536;
         }
         if (flength > 255)
         {
            length_lword[1] = (int)floor(flength / 256);
            flength -= length_lword[1] * 256;
         }
         length_lword[0] = (unsigned int)flength;

         for(i=0;i<4;i++)
         {
            fputc(length_lword[i],wavFile);
         }

         /* The file length for the RIFF chunk of the .wav file is in wavFlength */
         /* Write the file length in hi-byte/lo-byte, hi-word/lo-word format to the .wav file */

         fseek(wavFile,0x04,SEEK_SET);
         length_lword[3] = 0;
         if (wavFlength > 65535)
         {
            length_lword[2] = (unsigned int)floor(wavFlength / 65536);
            wavFlength -= length_lword[2] * 65536;
         }
         if (wavFlength > 256)
         {
            length_lword[1] = (unsigned int)floor(wavFlength / 256);
            wavFlength -= length_lword[1] * 256;
         }
         length_lword[0] = (unsigned int)wavFlength;
         for(i=0;i<4;i++)
         {
            fputc(length_lword[i],wavFile);
         }
         fseek(wavFile,0x2C,SEEK_SET);
      }
      if (wavFile)
      {
         fwrite( streambuffer.start, streambuffer.used, 1, wavFile );
         fflush( wavFile );
         fclose( wavFile );
      }
      if( parms.aflag )
      {
         paplay( streambuffer.start, streambuffer.used, parms.audioDevName );
      }
      strcpy(parms.wavFileName,"");
      currentBinFile++;
      parms.binFileQ = delq(parms.binFileQ);
      if (parms.dflag >= DISPLAY_FILENAME && parms.dflag < DISPLAY_DETAILS)
      {
         clearMeter(METER_LENGTH);
      }
   }
   streamclean( &streambuffer );
   return 0;
}
