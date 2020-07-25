/**
   this file is based upon:

   @file patest_sine8.c
   @ingroup test_src
   @brief Test 8 bit data: play a sine wave for several seconds.
   @author Ross Bencina <rossb@audiomulch.com>
*/
/*
 * $Id: patest_sine8.c 1748 2011-09-01 22:08:32Z philburk $
 *
 * This program uses the PortAudio Portable Audio Library.
 * For more information see: http://www.portaudio.com
 * Copyright (c) 1999-2000 Ross Bencina and Phil Burk
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files
 * (the "Software"), to deal in the Software without restriction,
 * including without limitation the rights to use, copy, modify, merge,
 * publish, distribute, sublicense, and/or sell copies of the Software,
 * and to permit persons to whom the Software is furnished to do so,
 * subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR
 * ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF
 * CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
 * WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

/*
 * The text above constitutes the entire PortAudio license; however,
 * the PortAudio community also makes the following non-binding requests:
 *
 * Any person wishing to distribute modifications to the Software is
 * requested to send the modifications to the original developer so that
 * they can be incorporated into the canonical version. It is also
 * requested that these non-binding requests be included along with the
 * license above.
 */

#include <stdio.h>
#include <string.h>
#include <math.h>
#include <portaudio.h>
#include "paplay.h"

#define SAMPLE_RATE   (44100)
#define TEST_UNSIGNED (0)

#if TEST_UNSIGNED
#define TEST_FORMAT   paUInt8
typedef unsigned char sample_t;
#define SILENCE       ((sample_t)0x80)
#else
#define TEST_FORMAT   paInt8
typedef char          sample_t;
#define SILENCE       ((sample_t)0x00)
#endif

struct playdata_s
{
   const unsigned char *bufferstart;
   int  size;
   int  played;
};

int palist()
{
   PaError             err;
   int                 i;
   int                 numDevices;
   const PaDeviceInfo  *deviceInfo;

   err = Pa_Initialize();
   if( err != paNoError )
   {
      goto error;
   }

   numDevices = Pa_GetDeviceCount();
   if( numDevices < 0 )
   {
      fprintf( stderr, "ERROR: Pa_CountDevices returned 0x%x\n", numDevices );
      err = numDevices;
      goto error;
   }

   for( i = 0; i < numDevices; i++ )
   {
      deviceInfo = Pa_GetDeviceInfo( i );
      fprintf( stdout, "device #%d: %s\n", i, deviceInfo->name );
   }

   Pa_Terminate();
   return err;

error:
   Pa_Terminate();
   fprintf( stderr, "An error occured while using the portaudio stream\n" );
   fprintf( stderr, "Error number: %d\n", err );
   fprintf( stderr, "Error message: %s\n", Pa_GetErrorText( err ) );
   return err;
}

/* This routine will be called by the PortAudio engine when audio is needed.
** It may called at interrupt level on some machines so don't do anything
** that could mess up the system like calling malloc() or free().
*/
static int pa_mem_callback( const void *inputBuffer, void *outputBuffer,
                            unsigned long framesPerBuffer,
                            const PaStreamCallbackTimeInfo* timeInfo,
                            PaStreamCallbackFlags statusFlags,
                            void *userData )
{
   char *out = (char*)outputBuffer;
   struct playdata_s *playdata = (struct playdata_s*)userData;
   int copysize = playdata->size - playdata->played;
   int finished = 0;
   (void) inputBuffer; /* Prevent unused variable warnings. */

   if( copysize > framesPerBuffer )
   {
      copysize = framesPerBuffer;
   }
   else
   {
      finished = 1;
   }
   memcpy( out, playdata->bufferstart + playdata->played, copysize );
   if( copysize < framesPerBuffer )
   {
      /* zero remainder of final buffer */
      memset( out + copysize, 0, framesPerBuffer - copysize );
   }
   playdata->played += copysize;

   return finished;
}

/*******************************************************************/
int paplay( const unsigned char *data, int size, const char *devicename )
{
   struct playdata_s   playdata;
   PaStreamParameters  outputParameters;
   PaStream*           stream;
   PaError             err;
   const PaDeviceInfo  *deviceInfo;
   int                 i;
   int                 numDevices;

   playdata.bufferstart = data;
   playdata.size        = size;
   playdata.played      = 0;

   err = Pa_Initialize();
   if( err != paNoError )
   {
      goto error;
   }

   numDevices = Pa_GetDeviceCount();
   if( numDevices < 0 )
   {
      fprintf( stderr, "ERROR: Pa_CountDevices returned 0x%x\n", numDevices );
      err = numDevices;
      goto error;
   }

   outputParameters.device = Pa_GetDefaultOutputDevice(); /* Default output device. */
   if( devicename )
   {
      int s = strlen( devicename );
      for( i=0; i<numDevices; i++ )
      {
         deviceInfo = Pa_GetDeviceInfo( i );
         if( devicename[s-1] == '*' )
         {
            if( !strncmp( devicename, deviceInfo->name, s - 1) )
            {
               outputParameters.device = i;
            }
         }
         else
         {
            if( !strcmp( devicename, deviceInfo->name ) )
            {
               outputParameters.device = i;
            }
         }
      }
   }
   if( outputParameters.device == paNoDevice )
   {
      fprintf(stderr,"Error: No default output device.\n");
      goto error;
   }
   deviceInfo = Pa_GetDeviceInfo( outputParameters.device );
   fprintf( stderr, "using device #%d: %s\n", outputParameters.device, deviceInfo->name );

   outputParameters.channelCount = 1;                     /* Mono output. */
   outputParameters.sampleFormat = TEST_FORMAT;
   outputParameters.suggestedLatency = Pa_GetDeviceInfo( outputParameters.device )->defaultLowOutputLatency;
   outputParameters.hostApiSpecificStreamInfo = NULL;
   err = Pa_OpenStream( &stream,
                        NULL,      /* No input. */
                        &outputParameters,
                        SAMPLE_RATE,
                        256,       /* Frames per buffer. */
                        paClipOff, /* We won't output out of range samples so don't bother clipping them. */
                        pa_mem_callback,
                        &playdata );
   if( err != paNoError )
   {
      goto error;
   }

   err = Pa_StartStream( stream );
   if( err != paNoError )
   {
      goto error;
   }

   while( ( err = Pa_IsStreamActive( stream ) ) == 1 )
   {
      Pa_Sleep(100);
   }
   if( err < 0 )
   {
      goto error;
   }

   err = Pa_CloseStream( stream );
   if( err != paNoError )
   {
      goto error;
   }

   Pa_Terminate();
   return err;

error:
   Pa_Terminate();
   fprintf( stderr, "An error occured while using the portaudio stream\n" );
   fprintf( stderr, "Error number: %d\n", err );
   fprintf( stderr, "Error message: %s\n", Pa_GetErrorText( err ) );
   return err;
}
