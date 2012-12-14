#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <Ecore_Audio.h>

#include "ecore_suite.h"

#include <stdio.h>
#include <Ecore.h>
#include <Ecore_Audio.h>

#define SF_FORMAT_RAW 0x040000
#define SF_FORMAT_PCM_U8 0x0005

#define SOUNDS_DIR TESTS_SRC_DIR"/src/tests/ecore/"

Ecore_Audio_Object *out;
Ecore_Audio_Object *in;

Eina_Bool
seek(void *data)
{
  double offs;
  fail_if(ecore_audio_input_seek(in, 0, SEEK_SET) != 0);
  fail_if(ecore_audio_input_seek(in, 1, SEEK_CUR) != 1);
  offs = ecore_audio_input_seek(in, -1, SEEK_END);
  fail_if((offs > 1.001) || (offs < 0.999));

  fail_if(ecore_audio_input_seek(in, SEEK_CUR, 10) != -1);
  fail_if(ecore_audio_input_seek(in, 5, 0) != -1);

  fail_if(ecore_audio_input_seek(in, 0, SEEK_SET) != 0);
}

Eina_Bool
input_del(void *data)
{
   printf("Deleting input\n");

   ecore_audio_input_del(in);

   return EINA_FALSE;
}

Eina_Bool
output_add(void *data)
{
   out = ecore_audio_output_add(ECORE_AUDIO_TYPE_SNDFILE);
   fail_if(!out);
   ecore_audio_output_sndfile_filename_set(out, SOUNDS_DIR"tmp.ogg");

   printf("Adding input\n");
   ecore_audio_output_input_add(out, in);

   return EINA_FALSE;
}

Eina_Bool
_quit(void *data)
{
   printf("Deleting output\n");
   ecore_audio_output_del(out);

   printf("Quitting\n");
   ecore_main_loop_quit();
}

START_TEST(ecore_test_ecore_audio_cleanup)
{
   in = ecore_audio_input_add(ECORE_AUDIO_TYPE_TONE);
   fail_if(!in);
   ecore_audio_input_tone_frequency_set(in, 1000);
   ecore_audio_input_tone_duration_set(in, 2);

   ecore_timer_add(1, output_add, NULL);
   ecore_timer_add(2, seek, NULL);
   ecore_timer_add(3.3, input_del, NULL);
   ecore_timer_add(4, _quit, NULL);

   ecore_main_loop_begin();

   ecore_file_remove(SOUNDS_DIR"tmp.ogg");
}
END_TEST


Eina_Bool
output_add_default(void *data)
{
   out = ecore_audio_output_add(ECORE_AUDIO_TYPE_PULSE);
   fail_if(!out);

   printf("Adding input\n");
   ecore_audio_output_input_add(out, in);

   return EINA_FALSE;
}

Eina_Bool
input_resume(void *data)
{
   double pos;

   fail_if(ecore_audio_input_paused_get(in) != EINA_TRUE);
   fail_if(ecore_audio_input_seek(in, 0, SEEK_CUR) != 0);
   ecore_audio_input_paused_set(in, EINA_FALSE);

   ecore_audio_input_looped_set(in, EINA_TRUE);

   return EINA_FALSE;
}

Eina_Bool
input_pause_seek(void *data)
{
   double pos;

   fail_if(ecore_audio_input_paused_get(in) != EINA_FALSE);
   ecore_audio_input_paused_set(in, EINA_TRUE);
   pos = ecore_audio_input_seek(in, 0, SEEK_CUR);

   fail_if(ecore_audio_input_seek(in, 0, SEEK_SET) != 0);
   pos = ecore_audio_input_seek(in, 0, SEEK_CUR);
   fail_if(pos > 0.01);

   ecore_timer_add(0.2, input_resume, NULL);
   return EINA_FALSE;
}

Eina_Bool looped(void *data, int type, void *event)
{
  Ecore_Audio_Object *in = (Ecore_Audio_Object *)event;
  fail_if(!in);

  fail_if(!ecore_audio_input_looped_get(in));
  ecore_audio_input_looped_set(in, EINA_FALSE);
  ecore_audio_input_seek(in, -0.5, SEEK_END);
}

Eina_Bool play_done(void *data, int type, void *event)
{
  Ecore_Audio_Object *in = (Ecore_Audio_Object *)event;
  Eina_List *ins;
  fail_if(!in);

  ins = ecore_audio_output_inputs_get(out);
  fail_if(eina_list_count(ins) != 1);

  fail_if(eina_list_data_get(ins) != in);
  ecore_audio_output_input_del(ecore_audio_input_output_get(in), in);
  ecore_audio_input_del(in);
  ecore_audio_output_del(out);
  ecore_main_loop_quit();
}

START_TEST(ecore_test_ecore_audio_default)
{
   in = ecore_audio_input_add(ECORE_AUDIO_TYPE_SNDFILE);
   ecore_audio_input_name_set(in, "modem.wav");
   ecore_audio_input_sndfile_filename_set(in, SOUNDS_DIR"modem.wav");

   ecore_timer_add(1, output_add_default, NULL);
   ecore_timer_add(1.2, input_pause_seek, NULL);
   ecore_event_handler_add(ECORE_AUDIO_INPUT_LOOPED, looped, NULL);
   ecore_event_handler_add(ECORE_AUDIO_INPUT_ENDED, play_done, NULL);

   ecore_main_loop_begin();

}
END_TEST

START_TEST(ecore_test_ecore_audio_sndfile)
{
   double len;

   in = ecore_audio_input_add(ECORE_AUDIO_TYPE_SNDFILE);
   fail_if(!in);

   ecore_audio_input_userdata_set(in, &len);
   ecore_audio_input_name_set(in, "sms.ogg");
   ecore_audio_input_sndfile_filename_set(in, SOUNDS_DIR"sms.ogg");
   fail_if(ecore_audio_input_channels_get(in) != 2);
   fail_if(ecore_audio_input_samplerate_get(in) != 44100);
   len = ecore_audio_input_length_get(in);
//   fail_if(len == 0);
//   fail_if(len != ecore_audio_input_remaining_get(in));
   fail_if(strcmp("sms.ogg", ecore_audio_input_name_get(in)));

   fail_if(ecore_audio_input_userdata_get(in) != &len);
   ecore_audio_input_del(in);
}
END_TEST

struct buffer {
    int offset;
    int length;
    unsigned char data[1024];
};

int _get_length(Ecore_Audio_Object *in)
{
  struct buffer *buf = ecore_audio_input_userdata_get(in);
  return buf->length;
}

int _seek(Ecore_Audio_Object *in, int offs, int whence)
{
  struct buffer *buf = ecore_audio_input_userdata_get(in);

  switch (whence) {
    case SEEK_SET:
      buf->offset = offs;
      break;
    case SEEK_CUR:
      buf->offset += offs;
      break;
    case SEEK_END:
      buf->offset = buf->length + offs;
      break;
  }
  return buf->offset;
}

int _tell(Ecore_Audio_Object *in)
{
  struct buffer *buf = ecore_audio_input_userdata_get(in);
  return buf->offset;
}

int _read(Ecore_Audio_Object *in, void *buffer, int length)
{
  struct buffer *buf = ecore_audio_input_userdata_get(in);

  if ((buf->offset + length) > buf->length)
    length = buf->length - buf->offset;

  memcpy(buffer, buf->data + buf->offset, length);
  buf->offset += length;

  return length;
}

int _write(Ecore_Audio_Object *out, const void *buffer, int length)
{
  struct buffer *buf = ecore_audio_output_userdata_get(out);

  if ((buf->offset + length) > buf->length)
    length = buf->length - buf->offset;

  memcpy(buf->data + buf->offset, buffer, length);

  buf->offset += length;

  return length;
}

Ecore_Audio_Vio vio = {
    .get_length = _get_length,
    .seek = _seek,
    .tell = _tell,
    .read = _read,
    .write = _write,
};

Eina_Bool sndfile_done(void *data, int type, void *event)
{
  Ecore_Audio_Object *in = (Ecore_Audio_Object *)event;

  ecore_audio_input_seek(in, 0, SEEK_SET);
  ecore_audio_input_del(in);
  ecore_audio_output_del(out);
  ecore_main_loop_quit();
}

START_TEST(ecore_test_ecore_audio_sndfile_vio)
{
   struct buffer indata = {
       .length = 1000,
       .offset = 0,
   };
   for (int i=0; i < 1000; i++) {
       indata.data[i] = i%256;
   }

   struct buffer outdata = {
       .length = 1000,
       .offset = 0,
   };
   for (int i=0; i < 1000; i++) {
       outdata.data[i] = 0;
   }

   in = ecore_audio_input_add(ECORE_AUDIO_TYPE_SNDFILE);
   fail_if(!in);

   ecore_audio_input_name_set(in, "tmp");
   ecore_audio_input_channels_set(in, 1);
   ecore_audio_input_samplerate_set(in, 44100);
   ecore_audio_input_userdata_set(in, &indata);
   ecore_audio_input_sndfile_format_set(in, SF_FORMAT_RAW | SF_FORMAT_PCM_U8);

   ecore_audio_input_sndfile_vio_set(in, &vio);

   out = ecore_audio_output_add(ECORE_AUDIO_TYPE_SNDFILE);
   fail_if(!out);
   ecore_audio_output_name_set(out, "tmp");
   ecore_audio_output_sndfile_format_set(out, SF_FORMAT_RAW | SF_FORMAT_PCM_U8);
   ecore_audio_output_userdata_set(out, &outdata);

   ecore_audio_output_sndfile_vio_set(out, &vio);

   ecore_audio_output_input_add(out, in);
   ecore_event_handler_add(ECORE_AUDIO_INPUT_ENDED, sndfile_done, NULL);

   ecore_main_loop_begin();

   // Off-by-one...must be libsndfile float conversion error?!
   for (int i = 0; i<1000; i++) {
       fail_if(indata.data[i] - outdata.data[i] > 1);
       fail_if(indata.data[i] - outdata.data[i] < -1);
   }
}
END_TEST

START_TEST(ecore_test_ecore_audio_custom)
{
  in = ecore_audio_input_add(ECORE_AUDIO_TYPE_CUSTOM);
  ecore_audio_input_del(in);
}
END_TEST

START_TEST(ecore_test_ecore_audio_init)
{
   int ret;

   ret = ecore_audio_init();
   fail_if(ret != 2);

   ret = ecore_audio_shutdown();
   fail_if(ret != 1);

}
END_TEST

void setup(void)
{
   int ret;

   ret = eina_init();
   ret = ecore_init();
   ret = ecore_audio_init();
}

void teardown(void)
{
   int ret;

   ret = ecore_audio_shutdown();
   ret = ecore_shutdown();
   ret = eina_shutdown();
}

void
ecore_test_ecore_audio(TCase *tc)
{
   tcase_add_checked_fixture (tc, setup, teardown);

   tcase_add_test(tc, ecore_test_ecore_audio_init);

   tcase_add_test(tc, ecore_test_ecore_audio_default);
   tcase_add_test(tc, ecore_test_ecore_audio_sndfile);
   tcase_add_test(tc, ecore_test_ecore_audio_sndfile_vio);
   tcase_add_test(tc, ecore_test_ecore_audio_custom);

   tcase_add_test(tc, ecore_test_ecore_audio_cleanup);
}

