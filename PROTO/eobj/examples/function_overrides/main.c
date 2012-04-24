#include "Eobj.h"
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
   eobj_init();

   Eobj *obj = eobj_add(INHERIT2_CLASS, NULL);

   eobj_do(obj, simple_a_set(1));
   Simple_Public_Data *pd = eobj_data_get(obj, SIMPLE_CLASS);
   fail_if(pd->a != 2);

   eobj_unref(obj);

   obj = eobj_add(INHERIT3_CLASS, NULL);

   eobj_do(obj, simple_a_set(1));
   pd = eobj_data_get(obj, SIMPLE_CLASS);
   fail_if(pd->a != 3);

   eobj_unref(obj);

   obj = eobj_add(INHERIT2_CLASS, NULL);
   eobj_do(obj, inherit2_print());
   eobj_unref(obj);

   obj = eobj_add(SIMPLE_CLASS, NULL);
   fail_if(eobj_do(obj, inherit2_print2()));

   fail_if(eobj_do_super(obj, simple_a_print()));

   eobj_constructor_super(obj);
   eobj_destructor_super(obj);

   eobj_unref(obj);

   eobj_shutdown();
   return 0;
}

