#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include <check.h>

#include "eet_suite.h"

START_TEST(eet_test_init)
{
   int ret;

   ret = eet_init();
   fail_if(ret != 1);

   ret = eet_shutdown();
   fail_if(ret != 0);
}
END_TEST

typedef struct _Eet_Test_Basic_Type Eet_Test_Basic_Type;
struct _Eet_Test_Basic_Type
{
   char c;
   short s;
   int i;
   long long l;
   char *str;
   char *istr;
   float f1;
   float f2;
   double d;
   unsigned char uc;
   unsigned short us;
   unsigned int ui;
   unsigned long long ul;
};

#define EET_TEST_CHAR 0x42
#define EET_TEST_SHORT 0x4224
#define EET_TEST_INT 0x42211224
#define EET_TEST_LONG_LONG 0x84CB42211224BC48
#define EET_TEST_STRING "my little test with escape \\\""
#define EET_TEST_KEY1 "key1"
#define EET_TEST_KEY2 "key2"
#define EET_TEST_FLOAT 123.45689
#define EET_TEST_FLOAT2 1.0
#define EET_TEST_FLOAT3 0.25
#define EET_TEST_FLOAT4 0.0001234
#define EET_TEST_DOUBLE 123456789.9876543210
#define EET_TEST_DOUBLE2 1.0
#define EET_TEST_DOUBLE3 0.25
#define EET_TEST_FILE_KEY1 "keys/data/1"
#define EET_TEST_FILE_KEY2 "keys/data/2"
#define EET_TEST_FILE_IMAGE "keys/images/"

typedef struct _Eet_Test_Image Eet_Test_Image;
struct _Eet_Test_Image
{
   unsigned int w;
   unsigned int h;
   unsigned int alpha;
   unsigned int color[];
};

static const Eet_Test_Image test_noalpha = {
  8, 8, 0,
  {
    0x00AA0000, 0x0000AA00, 0x000000AA, 0x00110000, 0x00AA0000, 0x0000AA00, 0x000000AA, 0x00110000,
    0x0000AA00, 0x000000AA, 0x00110000, 0x00AA0000, 0x0000AA00, 0x000000AA, 0x00110000, 0x00AA0000,
    0x000000AA, 0x00110000, 0x00AA0000, 0x0000AA00, 0x000000AA, 0x00110000, 0x00AA0000, 0x0000AA00,
    0x00110000, 0x00AA0000, 0x0000AA00, 0x000000AA, 0x00110000, 0x00AA0000, 0x0000AA00, 0x000000AA,
    0x00AA0000, 0x0000AA00, 0x000000AA, 0x00110000, 0x00AA0000, 0x0000AA00, 0x000000AA, 0x00110000,
    0x0000AA00, 0x000000AA, 0x00110000, 0x00AA0000, 0x0000AA00, 0x000000AA, 0x00110000, 0x00AA0000,
    0x000000AA, 0x00110000, 0x00AA0000, 0x0000AA00, 0x000000AA, 0x00110000, 0x00AA0000, 0x0000AA00,
    0x00110000, 0x00AA0000, 0x0000AA00, 0x000000AA, 0x00110000, 0x00AA0000, 0x0000AA00, 0x000000AA
  }
};

static const Eet_Test_Image test_alpha = {
  8, 8, 1,
  {
    0x0FAA0000, 0x0000AA00, 0x000000AA, 0x00110000, 0x00AA0000, 0x0000AA00, 0x000000AA, 0x0F110000,
    0x0000AA00, 0x0F0000AA, 0x00110000, 0x00AA0000, 0x0000AA00, 0x000000AA, 0x0F110000, 0x00AA0000,
    0x000000AA, 0x00110000, 0x0FAA0000, 0x0000AA00, 0x000000AA, 0x0F110000, 0x00AA0000, 0x0000AA00,
    0x00110000, 0x00AA0000, 0x0000AA00, 0x0F0000AA, 0x0F110000, 0x00AA0000, 0x0000AA00, 0x000000AA,
    0x00AA0000, 0x0000AA00, 0x000000AA, 0x0F110000, 0x0FAA0000, 0x0000AA00, 0x000000AA, 0x00110000,
    0x0000AA00, 0x000000AA, 0x0F110000, 0x00AA0000, 0x0000AA00, 0x0F0000AA, 0x00110000, 0x00AA0000,
    0x000000AA, 0x0F110000, 0x00AA0000, 0x0000AA00, 0x000000AA, 0x00110000, 0x0FAA0000, 0x0000AA00,
    0x0F110000, 0x00AA0000, 0x0000AA00, 0x000000AA, 0x00110000, 0x00AA0000, 0x0000AA00, 0x0F0000AA
  }
};


START_TEST(eet_test_basic_data_type_encoding_decoding)
{
   Eet_Data_Descriptor *edd;
   Eet_Test_Basic_Type *result;
   Eet_Data_Descriptor_Class eddc;
   Eet_Test_Basic_Type etbt;
   void *transfert;
   int size;
   float tmp;

   etbt.c = EET_TEST_CHAR;
   etbt.s = EET_TEST_SHORT;
   etbt.i = EET_TEST_INT;
   etbt.l = EET_TEST_LONG_LONG;
   etbt.str = EET_TEST_STRING;
   etbt.istr = EET_TEST_STRING;
   etbt.f1 = - EET_TEST_FLOAT;
   etbt.d = - EET_TEST_DOUBLE;
   etbt.f2 = EET_TEST_FLOAT4;
   etbt.uc = EET_TEST_CHAR;
   etbt.us = EET_TEST_SHORT;
   etbt.ui = EET_TEST_INT;
   etbt.ul = EET_TEST_LONG_LONG;

   eet_test_setup_eddc(&eddc);
   eddc.name = "Eet_Test_Basic_Type";
   eddc.size = sizeof(Eet_Test_Basic_Type);

   edd = eet_data_descriptor2_new(&eddc);
   fail_if(!edd);

   EET_DATA_DESCRIPTOR_ADD_BASIC(edd, Eet_Test_Basic_Type, "c", c, EET_T_CHAR);
   EET_DATA_DESCRIPTOR_ADD_BASIC(edd, Eet_Test_Basic_Type, "s", s, EET_T_SHORT);
   EET_DATA_DESCRIPTOR_ADD_BASIC(edd, Eet_Test_Basic_Type, "i", i, EET_T_INT);
   EET_DATA_DESCRIPTOR_ADD_BASIC(edd, Eet_Test_Basic_Type, "l", l, EET_T_LONG_LONG);
   EET_DATA_DESCRIPTOR_ADD_BASIC(edd, Eet_Test_Basic_Type, "str", str, EET_T_STRING);
   EET_DATA_DESCRIPTOR_ADD_BASIC(edd, Eet_Test_Basic_Type, "istr", istr, EET_T_INLINED_STRING);
   EET_DATA_DESCRIPTOR_ADD_BASIC(edd, Eet_Test_Basic_Type, "f1", f1, EET_T_FLOAT);
   EET_DATA_DESCRIPTOR_ADD_BASIC(edd, Eet_Test_Basic_Type, "f2", f2, EET_T_FLOAT);
   EET_DATA_DESCRIPTOR_ADD_BASIC(edd, Eet_Test_Basic_Type, "d", d, EET_T_DOUBLE);
   EET_DATA_DESCRIPTOR_ADD_BASIC(edd, Eet_Test_Basic_Type, "uc", uc, EET_T_UCHAR);
   EET_DATA_DESCRIPTOR_ADD_BASIC(edd, Eet_Test_Basic_Type, "us", us, EET_T_USHORT);
   EET_DATA_DESCRIPTOR_ADD_BASIC(edd, Eet_Test_Basic_Type, "ui", ui, EET_T_UINT);
   EET_DATA_DESCRIPTOR_ADD_BASIC(edd, Eet_Test_Basic_Type, "ul", ul, EET_T_ULONG_LONG);

   transfert = eet_data_descriptor_encode(edd, &etbt, &size);
   fail_if(!transfert || size <= 0);

   result = eet_data_descriptor_decode(edd, transfert, size);
   fail_if(!result);

   fail_if(result->c != EET_TEST_CHAR);
   fail_if(result->s != EET_TEST_SHORT);
   fail_if(result->i != EET_TEST_INT);
   fail_if(result->l != EET_TEST_LONG_LONG);
   fail_if(strcmp(result->str, EET_TEST_STRING) != 0);
   fail_if(strcmp(result->istr, EET_TEST_STRING) != 0);
   fail_if(result->uc != EET_TEST_CHAR);
   fail_if(result->us != EET_TEST_SHORT);
   fail_if(result->ui != EET_TEST_INT);
   fail_if(result->ul != EET_TEST_LONG_LONG);

   tmp = (result->f1 + EET_TEST_FLOAT);
   if (tmp < 0) tmp = -tmp;
   fail_if(tmp > 0.005);

   tmp = (result->f2 - EET_TEST_FLOAT4);
   if (tmp < 0) tmp = -tmp;
   fail_if(tmp > 0.005);

   tmp = (result->d + EET_TEST_DOUBLE);
   if (tmp < 0) tmp = -tmp;
   fail_if(tmp > 0.00005);

   free(result->str);
   free(result);

   eet_data_descriptor_free(edd);
}
END_TEST

typedef struct _Eet_Test_Ex_Type Eet_Test_Ex_Type;
struct _Eet_Test_Ex_Type
{
   char c;
   short s;
   int i;
   unsigned long long l;
   char *str;
   char *istr;
   float f1;
   float f2;
   float f3;
   float f4;
   double d1;
   double d2;
   double d3;
   double d4;
   Eet_List *list;
   Eet_Hash *hash;
   Eet_List *ilist;
   Eet_Hash *ihash;
   unsigned char uc;
   unsigned short us;
   unsigned int ui;
   unsigned long long ul;
};

static int i42 = 42;
static int i7 = 7;

static Eet_Test_Ex_Type*
_eet_test_ex_set(Eet_Test_Ex_Type *res, int offset)
{
   if (!res) res = malloc( sizeof(Eet_Test_Ex_Type));
   if (!res) return NULL;

   res->c = EET_TEST_CHAR + offset;
   res->s = EET_TEST_SHORT + offset;
   res->i = EET_TEST_INT + offset;
   res->l = EET_TEST_LONG_LONG + offset;
   res->str = EET_TEST_STRING;
   res->istr = EET_TEST_STRING;
   res->f1 = EET_TEST_FLOAT + offset;
   res->f2 = -(EET_TEST_FLOAT2 + offset);
   res->f3 = EET_TEST_FLOAT3 + offset;
   res->f4 = EET_TEST_FLOAT2 + offset;
   res->d1 = EET_TEST_DOUBLE + offset;
   res->d2 = -(EET_TEST_DOUBLE2 + offset);
   res->d3 = EET_TEST_DOUBLE3 + offset;
   res->d4 = EET_TEST_DOUBLE2 + offset;
   res->list = NULL;
   res->hash = NULL;
   res->ilist = NULL;
   res->ihash = NULL;
   res->uc = EET_TEST_CHAR + offset;
   res->us = EET_TEST_SHORT + offset;
   res->ui = EET_TEST_INT + offset;
   res->ul = EET_TEST_LONG_LONG + offset;

   return res;
}

static int
_eet_test_ex_check(Eet_Test_Ex_Type *stuff, int offset)
{
   double tmp;

   if (!stuff) return 1;

   if (stuff->c != EET_TEST_CHAR + offset) return 1;
   if (stuff->s != EET_TEST_SHORT + offset) return 1;
   if (stuff->i != EET_TEST_INT + offset) return 1;
   if (stuff->l != EET_TEST_LONG_LONG + offset) return 1;
   if (strcmp(stuff->str, EET_TEST_STRING) != 0) return 1;
   if (strcmp(stuff->istr, EET_TEST_STRING) != 0) return 1;

   tmp = stuff->f1 - (EET_TEST_FLOAT + offset);
   if (tmp < 0) tmp = -tmp;
   if (tmp > 0.005) return 1;

   tmp = stuff->d1 - (EET_TEST_DOUBLE + offset);
   if (tmp < 0) tmp = -tmp;
   if (tmp > 0.00005) return 1;

   if (stuff->f2 != - (EET_TEST_FLOAT2 + offset)) return 1;
   if (stuff->d2 != - (EET_TEST_DOUBLE2 + offset)) return 1;

   if (stuff->f3 != EET_TEST_FLOAT3 + offset) return 1;
   if (stuff->d3 != EET_TEST_DOUBLE3 + offset) return 1;

   if (stuff->f4 != EET_TEST_FLOAT2 + offset) return 1;
   if (stuff->d4 != EET_TEST_DOUBLE2 + offset) return 1;

   if (stuff->uc != EET_TEST_CHAR + offset) return 1;
   if (stuff->us != EET_TEST_SHORT + offset) return 1;
   if (stuff->ui != EET_TEST_INT + offset) return 1;
   if (stuff->ul != EET_TEST_LONG_LONG + offset) return 1;

   return 0;
}

static int
func(const Eet_Hash *hash, const char *key, void *data, void *fdata)
{
   int *res = fdata;

   if (strcmp(key, EET_TEST_KEY1) != 0
       && strcmp(key, EET_TEST_KEY2) != 0) *res = 1;
   if (_eet_test_ex_check(data, 2)) *res = 1;

   return 1;
}

static int
func7(const Eet_Hash *hash, const char *key, void *data, void *fdata)
{
   int *res = fdata;
   int *val;

   val = data;
   if (!val) *res = 1;
   if (*val != 7) *res = 1;

   return 1;
}

START_TEST(eet_test_data_type_encoding_decoding)
{
   Eet_Data_Descriptor *edd;
   Eet_Test_Ex_Type *result;
   Eet_Test_Ex_Type *tmp;
   void *transfert;
   Eet_Data_Descriptor_Class eddc;
   Eet_Test_Ex_Type etbt;
   int size;
   int test;

   _eet_test_ex_set(&etbt, 0);
   etbt.list = eet_list_prepend(etbt.list, _eet_test_ex_set(NULL, 1));
   etbt.hash = eet_hash_add(etbt.hash, EET_TEST_KEY1, _eet_test_ex_set(NULL, 2));
   etbt.ilist = eet_list_prepend(etbt.ilist, &i42);
   etbt.ihash = eet_hash_add(etbt.ihash, EET_TEST_KEY1, &i7);

   eet_test_setup_eddc(&eddc);
   eddc.name = "Eet_Test_Ex_Type";
   eddc.size = sizeof(Eet_Test_Ex_Type);

   edd = eet_data_descriptor3_new(&eddc);
   fail_if(!edd);

   EET_DATA_DESCRIPTOR_ADD_BASIC(edd, Eet_Test_Ex_Type, "c", c, EET_T_CHAR);
   EET_DATA_DESCRIPTOR_ADD_BASIC(edd, Eet_Test_Ex_Type, "s", s, EET_T_SHORT);
   EET_DATA_DESCRIPTOR_ADD_BASIC(edd, Eet_Test_Ex_Type, "i", i, EET_T_INT);
   EET_DATA_DESCRIPTOR_ADD_BASIC(edd, Eet_Test_Ex_Type, "l", l, EET_T_LONG_LONG);
   EET_DATA_DESCRIPTOR_ADD_BASIC(edd, Eet_Test_Ex_Type, "str", str, EET_T_STRING);
   EET_DATA_DESCRIPTOR_ADD_BASIC(edd, Eet_Test_Ex_Type, "istr", istr, EET_T_INLINED_STRING);
   EET_DATA_DESCRIPTOR_ADD_BASIC(edd, Eet_Test_Ex_Type, "f1", f1, EET_T_FLOAT);
   EET_DATA_DESCRIPTOR_ADD_BASIC(edd, Eet_Test_Ex_Type, "f2", f2, EET_T_FLOAT);
   EET_DATA_DESCRIPTOR_ADD_BASIC(edd, Eet_Test_Ex_Type, "f3", f3, EET_T_FLOAT);
   EET_DATA_DESCRIPTOR_ADD_BASIC(edd, Eet_Test_Ex_Type, "f4", f4, EET_T_FLOAT);
   EET_DATA_DESCRIPTOR_ADD_BASIC(edd, Eet_Test_Ex_Type, "d1", d1, EET_T_DOUBLE);
   EET_DATA_DESCRIPTOR_ADD_BASIC(edd, Eet_Test_Ex_Type, "d2", d2, EET_T_DOUBLE);
   EET_DATA_DESCRIPTOR_ADD_BASIC(edd, Eet_Test_Ex_Type, "d3", d3, EET_T_DOUBLE);
   EET_DATA_DESCRIPTOR_ADD_BASIC(edd, Eet_Test_Ex_Type, "d4", d4, EET_T_DOUBLE);
   EET_DATA_DESCRIPTOR_ADD_BASIC(edd, Eet_Test_Ex_Type, "uc", uc, EET_T_UCHAR);
   EET_DATA_DESCRIPTOR_ADD_BASIC(edd, Eet_Test_Ex_Type, "us", us, EET_T_USHORT);
   EET_DATA_DESCRIPTOR_ADD_BASIC(edd, Eet_Test_Ex_Type, "ui", ui, EET_T_UINT);
   EET_DATA_DESCRIPTOR_ADD_BASIC(edd, Eet_Test_Ex_Type, "ul", ul, EET_T_ULONG_LONG);
   EET_DATA_DESCRIPTOR_ADD_LIST(edd, Eet_Test_Ex_Type, "list", list, edd);
   EET_DATA_DESCRIPTOR_ADD_HASH(edd, Eet_Test_Ex_Type, "hash", hash, edd);
   eet_data_descriptor_element_add(edd, "ilist", EET_T_INT, EET_G_LIST,
				   (char *)(&(etbt.ilist)) - (char *)(&(etbt)),
				   0, NULL, NULL);
   eet_data_descriptor_element_add(edd, "ihash", EET_T_INT, EET_G_HASH,
				   (char *)(&(etbt.ihash)) - (char *)(&(etbt)),
				   0, NULL, NULL);

   transfert = eet_data_descriptor_encode(edd, &etbt, &size);
   fail_if(!transfert || size <= 0);

   result = eet_data_descriptor_decode(edd, transfert, size);
   fail_if(!result);

   fail_if(_eet_test_ex_check(result, 0) != 0);
   fail_if(_eet_test_ex_check(eet_list_data(result->list), 1) != 0);
   fail_if(eet_list_data(result->ilist) == NULL);
   fail_if(*((int*)eet_list_data(result->ilist)) != 42);

   test = 0;
   eet_hash_foreach(result->hash, func, &test);
   fail_if(test != 0);
   eet_hash_foreach(result->ihash, func7, &test);
   fail_if(test != 0);
}
END_TEST

static void
append_string(void *data, const char *str)
{
   char **string = data;
   int length;

   if (!data) return ;

   length = *string ? strlen(*string) : 0;
   *string = realloc(*string, strlen(str) + length + 1);

   memcpy((*string) + length, str, strlen(str) + 1);
}

START_TEST(eet_test_data_type_dump_undump)
{
   Eet_Data_Descriptor *edd;
   Eet_Test_Ex_Type *result;
   Eet_Test_Ex_Type *tmp;
   Eet_Data_Descriptor_Class eddc;
   Eet_Test_Ex_Type etbt;
   char *transfert1;
   char *transfert2;
   char *string1;
   char *string2;
   int size1;
   int size2;
   int test;

   int i;

   _eet_test_ex_set(&etbt, 0);
   etbt.list = eet_list_prepend(etbt.list, _eet_test_ex_set(NULL, 1));
   etbt.list = eet_list_prepend(etbt.list, _eet_test_ex_set(NULL, 1));
   etbt.hash = eet_hash_add(etbt.hash, EET_TEST_KEY1, _eet_test_ex_set(NULL, 2));
   etbt.hash = eet_hash_add(etbt.hash, EET_TEST_KEY2, _eet_test_ex_set(NULL, 2));
   etbt.ilist = eet_list_prepend(etbt.ilist, &i42);
   etbt.ilist = eet_list_prepend(etbt.ilist, &i42);
   etbt.ihash = eet_hash_add(etbt.ihash, EET_TEST_KEY1, &i7);
   etbt.ihash = eet_hash_add(etbt.ihash, EET_TEST_KEY2, &i7);

   eet_test_setup_eddc(&eddc);
   eddc.name = "Eet_Test_Ex_Type";
   eddc.size = sizeof(Eet_Test_Ex_Type);

   edd = eet_data_descriptor3_new(&eddc);
   fail_if(!edd);

   EET_DATA_DESCRIPTOR_ADD_BASIC(edd, Eet_Test_Ex_Type, "c", c, EET_T_CHAR);
   EET_DATA_DESCRIPTOR_ADD_BASIC(edd, Eet_Test_Ex_Type, "s", s, EET_T_SHORT);
   EET_DATA_DESCRIPTOR_ADD_BASIC(edd, Eet_Test_Ex_Type, "i", i, EET_T_INT);
   EET_DATA_DESCRIPTOR_ADD_BASIC(edd, Eet_Test_Ex_Type, "l", l, EET_T_LONG_LONG);
   EET_DATA_DESCRIPTOR_ADD_BASIC(edd, Eet_Test_Ex_Type, "str", str, EET_T_STRING);
   EET_DATA_DESCRIPTOR_ADD_BASIC(edd, Eet_Test_Ex_Type, "istr", istr, EET_T_INLINED_STRING);
   EET_DATA_DESCRIPTOR_ADD_BASIC(edd, Eet_Test_Ex_Type, "f1", f1, EET_T_FLOAT);
   EET_DATA_DESCRIPTOR_ADD_BASIC(edd, Eet_Test_Ex_Type, "f2", f2, EET_T_FLOAT);
   EET_DATA_DESCRIPTOR_ADD_BASIC(edd, Eet_Test_Ex_Type, "f3", f3, EET_T_FLOAT);
   EET_DATA_DESCRIPTOR_ADD_BASIC(edd, Eet_Test_Ex_Type, "f4", f4, EET_T_FLOAT);
   EET_DATA_DESCRIPTOR_ADD_BASIC(edd, Eet_Test_Ex_Type, "d1", d1, EET_T_DOUBLE);
   EET_DATA_DESCRIPTOR_ADD_BASIC(edd, Eet_Test_Ex_Type, "d2", d2, EET_T_DOUBLE);
   EET_DATA_DESCRIPTOR_ADD_BASIC(edd, Eet_Test_Ex_Type, "d3", d3, EET_T_DOUBLE);
   EET_DATA_DESCRIPTOR_ADD_BASIC(edd, Eet_Test_Ex_Type, "d4", d4, EET_T_DOUBLE);
   EET_DATA_DESCRIPTOR_ADD_BASIC(edd, Eet_Test_Ex_Type, "uc", uc, EET_T_UCHAR);
   EET_DATA_DESCRIPTOR_ADD_BASIC(edd, Eet_Test_Ex_Type, "us", us, EET_T_USHORT);
   EET_DATA_DESCRIPTOR_ADD_BASIC(edd, Eet_Test_Ex_Type, "ui", ui, EET_T_UINT);
   EET_DATA_DESCRIPTOR_ADD_BASIC(edd, Eet_Test_Ex_Type, "ul", ul, EET_T_ULONG_LONG);
   EET_DATA_DESCRIPTOR_ADD_LIST(edd, Eet_Test_Ex_Type, "list", list, edd);
   EET_DATA_DESCRIPTOR_ADD_HASH(edd, Eet_Test_Ex_Type, "hash", hash, edd);
   eet_data_descriptor_element_add(edd, "ilist", EET_T_INT, EET_G_LIST,
				   (char *)(&(etbt.ilist)) - (char *)(&(etbt)),
				   0, NULL, NULL);
   eet_data_descriptor_element_add(edd, "ihash", EET_T_INT, EET_G_HASH,
				   (char *)(&(etbt.ihash)) - (char *)(&(etbt)),
				   0, NULL, NULL);

   transfert1 = eet_data_descriptor_encode(edd, &etbt, &size1);
   fail_if(!transfert1 || size1 <= 0);

   string1 = NULL;
   eet_data_text_dump(transfert1, size1, append_string, &string1);
   fail_if(!string1);

   transfert2 = eet_data_text_undump(string1, strlen(string1), &size2);
   fail_if(!transfert2 && size2 <= 0);
   fail_if(size1 != size2);

   string2 = NULL;
   eet_data_text_dump(transfert2, size2, append_string, &string2);
   fail_if(!string2);

   fail_if(memcmp(transfert1, transfert2, size1) != 0);

   result = eet_data_descriptor_decode(edd, transfert2, size2);
   fail_if(!result);

   fail_if(_eet_test_ex_check(result, 0) != 0);
   fail_if(_eet_test_ex_check(eet_list_data(result->list), 1) != 0);
   fail_if(eet_list_data(result->ilist) == NULL);
   fail_if(*((int*)eet_list_data(result->ilist)) != 42);

   test = 0;
   eet_hash_foreach(result->hash, func, &test);
   fail_if(test != 0);
   eet_hash_foreach(result->ihash, func7, &test);
   fail_if(test != 0);
}
END_TEST

START_TEST(eet_file_simple_write)
{
   const char *buffer = "Here is a string of data to save !";
   Eet_File *ef;
   char *test;
   char *file = strdup("/tmp/eet_suite_testXXXXXX");
   int size;

   mktemp(file);

   fail_if(eet_mode_get(NULL) != EET_FILE_MODE_INVALID);

   ef = eet_open(file, EET_FILE_MODE_WRITE);
   fail_if(!ef);

   fail_if(!eet_write(ef, "keys/tests", buffer, sizeof(*buffer), 1));

   fail_if(eet_mode_get(ef) != EET_FILE_MODE_WRITE);

   fail_if(eet_list(ef, "*", &size) != NULL);
   fail_if(eet_num_entries(ef) != -1);

   eet_close(ef);

   /* Test read of simple file */
   ef = eet_open(file, EET_FILE_MODE_READ);
   fail_if(!ef);

   test = eet_read(ef, "keys/tests", &size);
   fail_if(!test);
   fail_if(size != sizeof(*buffer));

   fail_if(memcmp(test, buffer, sizeof(*buffer)) != 0);

   fail_if(eet_mode_get(ef) != EET_FILE_MODE_READ);
   fail_if(eet_num_entries(ef) != 1);

   eet_close(ef);

   /* Test eet cache system */
   ef = eet_open(file, EET_FILE_MODE_READ);
   fail_if(!ef);

   test = eet_read(ef, "keys/tests", &size);
   fail_if(!test);
   fail_if(size != sizeof(*buffer));

   fail_if(memcmp(test, buffer, sizeof(*buffer)) != 0);

   eet_close(ef);

   fail_if(unlink(file) != 0);
}
END_TEST

START_TEST(eet_file_data_test)
{
   const char *buffer = "Here is a string of data to save !";
   Eet_Data_Descriptor *edd;
   Eet_Test_Ex_Type *result;
   Eet_Test_Ex_Type *tmp;
   Eet_Dictionary *ed;
   Eet_File *ef;
   char **list;
   char *transfert1;
   char *transfert2;
   char *string1;
   char *string2;
   char *file = strdup("/tmp/eet_suite_testXXXXXX");
   Eet_Data_Descriptor_Class eddc;
   Eet_Test_Ex_Type etbt;
   int size;
   int size1;
   int size2;
   int test;

   int i;

   _eet_test_ex_set(&etbt, 0);
   etbt.list = eet_list_prepend(etbt.list, _eet_test_ex_set(NULL, 1));
   etbt.list = eet_list_prepend(etbt.list, _eet_test_ex_set(NULL, 1));
   etbt.hash = eet_hash_add(etbt.hash, EET_TEST_KEY1, _eet_test_ex_set(NULL, 2));
   etbt.hash = eet_hash_add(etbt.hash, EET_TEST_KEY2, _eet_test_ex_set(NULL, 2));
   etbt.ilist = eet_list_prepend(etbt.ilist, &i42);
   etbt.ilist = eet_list_prepend(etbt.ilist, &i42);
   etbt.ihash = eet_hash_add(etbt.ihash, EET_TEST_KEY1, &i7);
   etbt.ihash = eet_hash_add(etbt.ihash, EET_TEST_KEY2, &i7);

   eet_test_setup_eddc(&eddc);
   eddc.name = "Eet_Test_Ex_Type";
   eddc.size = sizeof(Eet_Test_Ex_Type);

   edd = eet_data_descriptor3_new(&eddc);
   fail_if(!edd);

   EET_DATA_DESCRIPTOR_ADD_BASIC(edd, Eet_Test_Ex_Type, "c", c, EET_T_CHAR);
   EET_DATA_DESCRIPTOR_ADD_BASIC(edd, Eet_Test_Ex_Type, "s", s, EET_T_SHORT);
   EET_DATA_DESCRIPTOR_ADD_BASIC(edd, Eet_Test_Ex_Type, "i", i, EET_T_INT);
   EET_DATA_DESCRIPTOR_ADD_BASIC(edd, Eet_Test_Ex_Type, "l", l, EET_T_LONG_LONG);
   EET_DATA_DESCRIPTOR_ADD_BASIC(edd, Eet_Test_Ex_Type, "str", str, EET_T_STRING);
   EET_DATA_DESCRIPTOR_ADD_BASIC(edd, Eet_Test_Ex_Type, "istr", istr, EET_T_INLINED_STRING);
   EET_DATA_DESCRIPTOR_ADD_BASIC(edd, Eet_Test_Ex_Type, "f1", f1, EET_T_FLOAT);
   EET_DATA_DESCRIPTOR_ADD_BASIC(edd, Eet_Test_Ex_Type, "f2", f2, EET_T_FLOAT);
   EET_DATA_DESCRIPTOR_ADD_BASIC(edd, Eet_Test_Ex_Type, "f3", f3, EET_T_FLOAT);
   EET_DATA_DESCRIPTOR_ADD_BASIC(edd, Eet_Test_Ex_Type, "f4", f4, EET_T_FLOAT);
   EET_DATA_DESCRIPTOR_ADD_BASIC(edd, Eet_Test_Ex_Type, "d1", d1, EET_T_DOUBLE);
   EET_DATA_DESCRIPTOR_ADD_BASIC(edd, Eet_Test_Ex_Type, "d2", d2, EET_T_DOUBLE);
   EET_DATA_DESCRIPTOR_ADD_BASIC(edd, Eet_Test_Ex_Type, "d3", d3, EET_T_DOUBLE);
   EET_DATA_DESCRIPTOR_ADD_BASIC(edd, Eet_Test_Ex_Type, "d4", d4, EET_T_DOUBLE);
   EET_DATA_DESCRIPTOR_ADD_BASIC(edd, Eet_Test_Ex_Type, "uc", uc, EET_T_UCHAR);
   EET_DATA_DESCRIPTOR_ADD_BASIC(edd, Eet_Test_Ex_Type, "us", us, EET_T_USHORT);
   EET_DATA_DESCRIPTOR_ADD_BASIC(edd, Eet_Test_Ex_Type, "ui", ui, EET_T_UINT);
   EET_DATA_DESCRIPTOR_ADD_BASIC(edd, Eet_Test_Ex_Type, "ul", ul, EET_T_ULONG_LONG);
   EET_DATA_DESCRIPTOR_ADD_LIST(edd, Eet_Test_Ex_Type, "list", list, edd);
   EET_DATA_DESCRIPTOR_ADD_HASH(edd, Eet_Test_Ex_Type, "hash", hash, edd);
   eet_data_descriptor_element_add(edd, "ilist", EET_T_INT, EET_G_LIST,
				   (char *)(&(etbt.ilist)) - (char *)(&(etbt)),
				   0, NULL, NULL);
   eet_data_descriptor_element_add(edd, "ihash", EET_T_INT, EET_G_HASH,
				   (char *)(&(etbt.ihash)) - (char *)(&(etbt)),
				   0, NULL, NULL);

   mktemp(file);

   /* Insert an error in etbt. */
   etbt.i = 0;

   /* Save the encoded data in a file. */
   ef = eet_open(file, EET_FILE_MODE_READ_WRITE);
   fail_if(!ef);

   fail_if(!eet_data_write(ef, edd, EET_TEST_FILE_KEY1, &etbt, 1));

   result = eet_data_read(ef, edd, EET_TEST_FILE_KEY1);
   fail_if(!result);

   fail_if(eet_mode_get(ef) != EET_FILE_MODE_READ_WRITE);

   /* Test string space. */
   ed = eet_dictionary_get(ef);

   fail_if(!eet_dictionary_string_check(ed, result->str));
   fail_if(eet_dictionary_string_check(ed, result->istr));

   eet_close(ef);

   /* Attempt to replace etbt by the correct one. */
   etbt.i = EET_TEST_INT;

   ef = eet_open(file, EET_FILE_MODE_READ_WRITE);
   fail_if(!ef);

   fail_if(!eet_data_write(ef, edd, EET_TEST_FILE_KEY1, &etbt, 0));

   eet_close(ef);

   /* Read back the data. */
   ef = eet_open(file, EET_FILE_MODE_READ_WRITE);
   fail_if(!ef);

   fail_if(!eet_data_write(ef, edd, EET_TEST_FILE_KEY2, &etbt, 1));

   result = eet_data_read(ef, edd, EET_TEST_FILE_KEY1);
   fail_if(!result);

   /* Test string space. */
   ed = eet_dictionary_get(ef);
   fail_if(!ed);

   fail_if(!eet_dictionary_string_check(ed, result->str));
   fail_if(eet_dictionary_string_check(ed, result->istr));

   /* Test the resulting data. */
   fail_if(_eet_test_ex_check(result, 0) != 0);
   fail_if(_eet_test_ex_check(eet_list_data(result->list), 1) != 0);
   fail_if(eet_list_data(result->ilist) == NULL);
   fail_if(*((int*)eet_list_data(result->ilist)) != 42);

   test = 0;
   eet_hash_foreach(result->hash, func, &test);
   fail_if(test != 0);
   eet_hash_foreach(result->ihash, func7, &test);
   fail_if(test != 0);

   list = eet_list(ef, "keys/*", &size);
   fail_if(eet_num_entries(ef) != 2);
   fail_if(size != 2);
   fail_if(!(strcmp(list[0], EET_TEST_FILE_KEY1) == 0 && strcmp(list[1], EET_TEST_FILE_KEY2) == 0)
	   && !(strcmp(list[0], EET_TEST_FILE_KEY2) == 0 && strcmp(list[1], EET_TEST_FILE_KEY1) == 0));
   free(list);

   fail_if(eet_delete(ef, NULL) != 0);
   fail_if(eet_delete(NULL, EET_TEST_FILE_KEY1) != 0);
   fail_if(eet_delete(ef, EET_TEST_FILE_KEY1) == 0);

   list = eet_list(ef, "keys/*", &size);
   fail_if(size != 1);
   fail_if(eet_num_entries(ef) != 1);

   /* Test some more wrong case */
   fail_if(eet_data_read(ef, edd, "plop") != NULL);
   fail_if(eet_data_read(ef, edd, EET_TEST_FILE_KEY1) != NULL);

   /* Reinsert and reread data */
   fail_if(!eet_data_write(ef, edd, EET_TEST_FILE_KEY1, &etbt, 0));
   fail_if(eet_data_read(ef, edd, EET_TEST_FILE_KEY1) == NULL);
   fail_if(eet_read_direct(ef, EET_TEST_FILE_KEY1, &size) == NULL);

   eet_close(ef);

   fail_if(unlink(file) != 0);
}
END_TEST

START_TEST(eet_file_data_dump_test)
{
   const char *buffer = "Here is a string of data to save !";
   Eet_Data_Descriptor *edd;
   Eet_Test_Ex_Type *result;
   Eet_Test_Ex_Type *tmp;
   Eet_Data_Descriptor_Class eddc;
   Eet_Test_Ex_Type etbt;
   Eet_File *ef;
   char *transfert1;
   char *transfert2;
   char *string1;
   char *string2;
   char *file = strdup("/tmp/eet_suite_testXXXXXX");
   int size;
   int size1;
   int size2;
   int test;

   int i;

   _eet_test_ex_set(&etbt, 0);
   etbt.list = eet_list_prepend(etbt.list, _eet_test_ex_set(NULL, 1));
   etbt.list = eet_list_prepend(etbt.list, _eet_test_ex_set(NULL, 1));
   etbt.hash = eet_hash_add(etbt.hash, EET_TEST_KEY1, _eet_test_ex_set(NULL, 2));
   etbt.hash = eet_hash_add(etbt.hash, EET_TEST_KEY2, _eet_test_ex_set(NULL, 2));
   etbt.ilist = eet_list_prepend(etbt.ilist, &i42);
   etbt.ilist = eet_list_prepend(etbt.ilist, &i42);
   etbt.ihash = eet_hash_add(etbt.ihash, EET_TEST_KEY1, &i7);
   etbt.ihash = eet_hash_add(etbt.ihash, EET_TEST_KEY2, &i7);

   eet_test_setup_eddc(&eddc);
   eddc.name = "Eet_Test_Ex_Type";
   eddc.size = sizeof(Eet_Test_Ex_Type);

   edd = eet_data_descriptor3_new(&eddc);
   fail_if(!edd);

   EET_DATA_DESCRIPTOR_ADD_BASIC(edd, Eet_Test_Ex_Type, "c", c, EET_T_CHAR);
   EET_DATA_DESCRIPTOR_ADD_BASIC(edd, Eet_Test_Ex_Type, "s", s, EET_T_SHORT);
   EET_DATA_DESCRIPTOR_ADD_BASIC(edd, Eet_Test_Ex_Type, "i", i, EET_T_INT);
   EET_DATA_DESCRIPTOR_ADD_BASIC(edd, Eet_Test_Ex_Type, "l", l, EET_T_LONG_LONG);
   EET_DATA_DESCRIPTOR_ADD_BASIC(edd, Eet_Test_Ex_Type, "str", str, EET_T_STRING);
   EET_DATA_DESCRIPTOR_ADD_BASIC(edd, Eet_Test_Ex_Type, "istr", istr, EET_T_INLINED_STRING);
   EET_DATA_DESCRIPTOR_ADD_BASIC(edd, Eet_Test_Ex_Type, "f1", f1, EET_T_FLOAT);
   EET_DATA_DESCRIPTOR_ADD_BASIC(edd, Eet_Test_Ex_Type, "f2", f2, EET_T_FLOAT);
   EET_DATA_DESCRIPTOR_ADD_BASIC(edd, Eet_Test_Ex_Type, "f3", f3, EET_T_FLOAT);
   EET_DATA_DESCRIPTOR_ADD_BASIC(edd, Eet_Test_Ex_Type, "f4", f4, EET_T_FLOAT);
   EET_DATA_DESCRIPTOR_ADD_BASIC(edd, Eet_Test_Ex_Type, "d1", d1, EET_T_DOUBLE);
   EET_DATA_DESCRIPTOR_ADD_BASIC(edd, Eet_Test_Ex_Type, "d2", d2, EET_T_DOUBLE);
   EET_DATA_DESCRIPTOR_ADD_BASIC(edd, Eet_Test_Ex_Type, "d3", d3, EET_T_DOUBLE);
   EET_DATA_DESCRIPTOR_ADD_BASIC(edd, Eet_Test_Ex_Type, "d4", d4, EET_T_DOUBLE);
   EET_DATA_DESCRIPTOR_ADD_BASIC(edd, Eet_Test_Ex_Type, "uc", uc, EET_T_UCHAR);
   EET_DATA_DESCRIPTOR_ADD_BASIC(edd, Eet_Test_Ex_Type, "us", us, EET_T_USHORT);
   EET_DATA_DESCRIPTOR_ADD_BASIC(edd, Eet_Test_Ex_Type, "ui", ui, EET_T_UINT);
   EET_DATA_DESCRIPTOR_ADD_BASIC(edd, Eet_Test_Ex_Type, "ul", ul, EET_T_ULONG_LONG);
   EET_DATA_DESCRIPTOR_ADD_LIST(edd, Eet_Test_Ex_Type, "list", list, edd);
   EET_DATA_DESCRIPTOR_ADD_HASH(edd, Eet_Test_Ex_Type, "hash", hash, edd);
   eet_data_descriptor_element_add(edd, "ilist", EET_T_INT, EET_G_LIST,
				   (char *)(&(etbt.ilist)) - (char *)(&(etbt)),
				   0, NULL, NULL);
   eet_data_descriptor_element_add(edd, "ihash", EET_T_INT, EET_G_HASH,
				   (char *)(&(etbt.ihash)) - (char *)(&(etbt)),
				   0, NULL, NULL);

   mktemp(file);

   /* Save the encoded data in a file. */
   ef = eet_open(file, EET_FILE_MODE_WRITE);
   fail_if(!ef);

   fail_if(!eet_data_write(ef, edd, EET_TEST_FILE_KEY1, &etbt, 1));

   eet_close(ef);

   /* Use dump/undump in the middle */
   ef = eet_open(file, EET_FILE_MODE_READ_WRITE);
   fail_if(!ef);

   string1 = NULL;
   fail_if(eet_data_dump(ef, EET_TEST_FILE_KEY1, append_string, &string1) != 1);
   fail_if(eet_delete(ef, EET_TEST_FILE_KEY1) == 0);
   fail_if(!eet_data_undump(ef, EET_TEST_FILE_KEY1, string1, strlen(string1), 1));

   eet_close(ef);

   /* Test the correctness of the reinsertion. */
   ef = eet_open(file, EET_FILE_MODE_READ);
   fail_if(!ef);

   result = eet_data_read(ef, edd, EET_TEST_FILE_KEY1);
   fail_if(!transfert1);

   eet_close(ef);

   /* Test the resulting data. */
   fail_if(_eet_test_ex_check(result, 0) != 0);
   fail_if(_eet_test_ex_check(eet_list_data(result->list), 1) != 0);
   fail_if(eet_list_data(result->ilist) == NULL);
   fail_if(*((int*)eet_list_data(result->ilist)) != 42);

   test = 0;
   eet_hash_foreach(result->hash, func, &test);
   fail_if(test != 0);
   eet_hash_foreach(result->ihash, func7, &test);
   fail_if(test != 0);

   fail_if(unlink(file) != 0);
}
END_TEST

START_TEST(eet_image)
{
   Eet_File *ef;
   char *file = strdup("/tmp/eet_suite_testXXXXXX");
   int *data;
   int compress;
   int quality;
   int result;
   int lossy;
   int alpha;
   int w;
   int h;

   mktemp(file);

   /* Save the encoded data in a file. */
   ef = eet_open(file, EET_FILE_MODE_READ_WRITE);
   fail_if(!ef);

   result = eet_data_image_write(ef, EET_TEST_FILE_IMAGE "0", test_noalpha.color,
				 test_noalpha.w, test_noalpha.h, test_noalpha.alpha,
				 0, 100, 0);
   fail_if(result == 0);

   result = eet_data_image_write(ef, EET_TEST_FILE_IMAGE "1", test_noalpha.color,
				 test_noalpha.w, test_noalpha.h, test_noalpha.alpha,
				 5, 100, 0);
   fail_if(result == 0);

   result = eet_data_image_write(ef, EET_TEST_FILE_IMAGE "2", test_noalpha.color,
				 test_noalpha.w, test_noalpha.h, test_noalpha.alpha,
				 9, 100, 0);
   fail_if(result == 0);

   result = eet_data_image_write(ef, EET_TEST_FILE_IMAGE "3", test_noalpha.color,
				 test_noalpha.w, test_noalpha.h, test_noalpha.alpha,
				 0, 100, 1);
   fail_if(result == 0);

   result = eet_data_image_write(ef, EET_TEST_FILE_IMAGE "4", test_noalpha.color,
				 test_noalpha.w, test_noalpha.h, test_noalpha.alpha,
				 0, 60, 1);
   fail_if(result == 0);

   result = eet_data_image_write(ef, EET_TEST_FILE_IMAGE "5", test_noalpha.color,
				 test_noalpha.w, test_noalpha.h, test_noalpha.alpha,
				 0, 10, 1);
   fail_if(result == 0);

   result = eet_data_image_write(ef, EET_TEST_FILE_IMAGE "6", test_noalpha.color,
				 test_noalpha.w, test_noalpha.h, test_noalpha.alpha,
				 0, 0, 1);
   fail_if(result == 0);

   result = eet_data_image_write(ef, EET_TEST_FILE_IMAGE "7", test_alpha.color,
				 test_alpha.w, test_alpha.h, test_alpha.alpha,
				 9, 100, 0);
   fail_if(result == 0);

   result = eet_data_image_write(ef, EET_TEST_FILE_IMAGE "8", test_alpha.color,
				 test_alpha.w, test_alpha.h, test_alpha.alpha,
				 0, 80, 1);
   fail_if(result == 0);

   result = eet_data_image_write(ef, EET_TEST_FILE_IMAGE "9", test_alpha.color,
				 test_alpha.w, test_alpha.h, test_alpha.alpha,
				 0, 100, 1);
   fail_if(result == 0);

   data = eet_data_image_read(ef, EET_TEST_FILE_IMAGE "2", &w, &h, &alpha, &compress, &quality, &lossy);
   fail_if(data == NULL);
   fail_if(w != test_noalpha.w);
   fail_if(h != test_noalpha.h);
   fail_if(alpha != test_noalpha.alpha);
   fail_if(compress != 9);
   fail_if(lossy != 0);
   fail_if(data[0] != test_noalpha.color[0]);
   free(data);

   result = eet_data_image_header_read(ef, EET_TEST_FILE_IMAGE "2", &w, &h, &alpha, &compress, &quality, &lossy);
   fail_if(result == 0);
   fail_if(w != test_noalpha.w);
   fail_if(h != test_noalpha.h);
   fail_if(alpha != test_noalpha.alpha);
   fail_if(compress != 9);
   fail_if(lossy != 0);

   eet_close(ef);

   /* Test read of image */
   ef = eet_open(file, EET_FILE_MODE_READ);
   fail_if(!ef);

   result = eet_data_image_header_read(ef, EET_TEST_FILE_IMAGE "0", &w, &h, &alpha, &compress, &quality, &lossy);
   fail_if(result == 0);
   fail_if(w != test_noalpha.w);
   fail_if(h != test_noalpha.h);
   fail_if(alpha != test_noalpha.alpha);
   fail_if(compress != 0);
   fail_if(lossy != 0);

   data = eet_data_image_read(ef, EET_TEST_FILE_IMAGE "0", &w, &h, &alpha, &compress, &quality, &lossy);
   fail_if(data == NULL);
   fail_if(w != test_noalpha.w);
   fail_if(h != test_noalpha.h);
   fail_if(alpha != test_noalpha.alpha);
   fail_if(compress != 0);
   fail_if(quality != 100);
   fail_if(lossy != 0);
   fail_if(data[0] != test_noalpha.color[0]);
   free(data);

   data = eet_data_image_read(ef, EET_TEST_FILE_IMAGE "1", &w, &h, &alpha, &compress, &quality, &lossy);
   fail_if(data == NULL);
   fail_if(w != test_noalpha.w);
   fail_if(h != test_noalpha.h);
   fail_if(alpha != test_noalpha.alpha);
   fail_if(compress != 5);
   fail_if(quality != 100);
   fail_if(lossy != 0);
   fail_if(data[0] != test_noalpha.color[0]);
   free(data);

   data = eet_data_image_read(ef, EET_TEST_FILE_IMAGE "2", &w, &h, &alpha, &compress, &quality, &lossy);
   fail_if(data == NULL);
   fail_if(w != test_noalpha.w);
   fail_if(h != test_noalpha.h);
   fail_if(alpha != test_noalpha.alpha);
   fail_if(compress != 9);
   fail_if(lossy != 0);
   fail_if(data[0] != test_noalpha.color[0]);
   free(data);

   data = eet_data_image_read(ef, EET_TEST_FILE_IMAGE "3", &w, &h, &alpha, &compress, &quality, &lossy);
   fail_if(data == NULL);
   fail_if(w != test_noalpha.w);
   fail_if(h != test_noalpha.h);
   fail_if(alpha != test_noalpha.alpha);
   fail_if(lossy != 1);
   free(data);

   data = eet_data_image_read(ef, EET_TEST_FILE_IMAGE "5", &w, &h, &alpha, &compress, &quality, &lossy);
   fail_if(data == NULL);
   fail_if(w != test_noalpha.w);
   fail_if(h != test_noalpha.h);
   fail_if(alpha != test_noalpha.alpha);
   fail_if(lossy != 1);
   free(data);

   data = eet_data_image_read(ef, EET_TEST_FILE_IMAGE "6", &w, &h, &alpha, &compress, &quality, &lossy);
   fail_if(data == NULL);
   fail_if(w != test_noalpha.w);
   fail_if(h != test_noalpha.h);
   fail_if(alpha != test_noalpha.alpha);
   fail_if(lossy != 1);
   free(data);

   result = eet_data_image_header_read(ef, EET_TEST_FILE_IMAGE "7", &w, &h, &alpha, &compress, &quality, &lossy);
   fail_if(result == 0);
   fail_if(w != test_alpha.w);
   fail_if(h != test_alpha.h);
   fail_if(alpha != test_alpha.alpha);
   fail_if(compress != 9);
   fail_if(lossy != 0);

   data = eet_data_image_read(ef, EET_TEST_FILE_IMAGE "7", &w, &h, &alpha, &compress, &quality, &lossy);
   fail_if(data == NULL);
   fail_if(w != test_alpha.w);
   fail_if(h != test_alpha.h);
   fail_if(alpha != test_alpha.alpha);
   fail_if(compress != 9);
   fail_if(lossy != 0);
   fail_if(data[0] != test_alpha.color[0]);
   free(data);

   result = eet_data_image_header_read(ef, EET_TEST_FILE_IMAGE "9", &w, &h, &alpha, &compress, &quality, &lossy);
   fail_if(result == 0);
   fail_if(w != test_alpha.w);
   fail_if(h != test_alpha.h);
   fail_if(alpha != test_alpha.alpha);
   fail_if(lossy != 1);

   data = eet_data_image_read(ef, EET_TEST_FILE_IMAGE "9", &w, &h, &alpha, &compress, &quality, &lossy);
   fail_if(data == NULL);
   fail_if(w != test_alpha.w);
   fail_if(h != test_alpha.h);
   fail_if(alpha != test_alpha.alpha);
   fail_if(lossy != 1);
   free(data);

   eet_close(ef);
}
END_TEST

Suite *
eet_suite(void)
{
   Suite *s;
   TCase *tc;

   s = suite_create("Eet");

   tc = tcase_create("Eet_Init");
   tcase_add_test(tc, eet_test_init);
   suite_add_tcase(s, tc);

   tc = tcase_create("Eet Data Encoding/Decoding");
   tcase_add_test(tc, eet_test_basic_data_type_encoding_decoding);
   tcase_add_test(tc, eet_test_data_type_encoding_decoding);
   tcase_add_test(tc, eet_test_data_type_dump_undump);
   suite_add_tcase(s, tc);

   tc = tcase_create("Eet File");
   tcase_add_test(tc, eet_file_simple_write);
   tcase_add_test(tc, eet_file_data_test);
   tcase_add_test(tc, eet_file_data_dump_test);
   suite_add_tcase(s, tc);

   tc = tcase_create("Eet Image");
   tcase_add_test(tc, eet_image);
   suite_add_tcase(s, tc);

   return s;
}

int
main(void)
{
   Suite   *s;
   SRunner *sr;
   int      failed_count;

   s = eet_suite();
   sr = srunner_create(s);
   srunner_run_all(sr, CK_NORMAL);
   failed_count = srunner_ntests_failed(sr);
   srunner_free(sr);

   return (failed_count == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
