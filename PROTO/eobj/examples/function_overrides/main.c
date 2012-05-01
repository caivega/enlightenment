#include "Eo.h"
#include "simple.h"
#include "inherit.h"
#include "inherit2.h"
#include "inherit3.h"

#include "../eunit_tests.h"

int
main(int argc, char *argv[])
{
   (void) argc;
   (void) argv;
   eo_init();

   Eo *obj = eo_add(INHERIT2_CLASS, NULL);

   eo_do(obj, simple_a_set(1));
   Simple_Public_Data *pd = eo_data_get(obj, SIMPLE_CLASS);
   fail_if(pd->a != 2);

   eo_unref(obj);

   obj = eo_add(INHERIT3_CLASS, NULL);

   eo_do(obj, simple_a_set(1));
   pd = eo_data_get(obj, SIMPLE_CLASS);
   fail_if(pd->a != 3);

   eo_unref(obj);

   obj = eo_add(INHERIT2_CLASS, NULL);
   eo_do(obj, inherit2_print());
   eo_unref(obj);

   obj = eo_add(SIMPLE_CLASS, NULL);
   fail_if(eo_do(obj, inherit2_print2()));

   fail_if(eo_do_super(obj, simple_a_print()));

   eo_constructor_super(obj);
   eo_destructor_super(obj);

   eo_unref(obj);

   eo_shutdown();
   return 0;
}

