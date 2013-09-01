/*
//-----------------------------------------------------------------------------
// The confidential and proprietary information contained in this file may
// only be used by a person authorised under and to the extent permitted
// by a subsisting licensing agreement from ARM Limited.
//
//            (C) COPYRIGHT 2008-2011  ARM Limited.
//                ALL RIGHTS RESERVED
//
// This entire notice must be reproduced on all copies of this file
// and copies of this file may only be made by a person if such person is
// permitted to do so under the terms of a subsisting license agreement
// from ARM Limited.
//
//      SVN Information
//
//      Checked In          : $Date: 2009-12-07 16:50:12 +0000 (Mon, 07 Dec 2009) $
//
//      Revision            : $Revision: 126049 $
//
//      Release Information : CORTEX-A7-r0p0-00rel0
//
//-----------------------------------------------------------------------------
*/

/*
 ****************************************************************************
 *
 *                   "DHRYSTONE" Benchmark Program
 *                   -----------------------------
 *                                                                            
 *  Version:    C, Version 2.1
 *                                                                            
 *  File:       dhry_1.c (part 2 of 3)
 *
 *  Date:       May 25, 1988
 *
 *  Author:     Reinhold P. Weicker
 *
 ****************************************************************************
 */

#include "dhry.h"
//#include <linux/stdlib.h>  //mark by KT
#include <linux/vmalloc.h>  //add by KT

//add by KT
#define printf printk   
#define malloc vmalloc
#define times do_sys_times

/* Global Variables: */

Rec_Pointer     Ptr_Glob,
                Next_Ptr_Glob;
int             Int_Glob;
Boolean         Bool_Glob;
char            Ch_1_Glob,
                Ch_2_Glob;
int             Arr_1_Glob [50];
int             Arr_2_Glob [50] [50];

#ifndef REG
        Boolean Reg = false;
#define REG
        /* REG becomes defined as empty */
        /* i.e. no register variables   */
#else
        Boolean Reg = true;
#endif

/* variables for time measurement: */
#ifdef TIMES
struct tms      time_info;
/*extern  int     times ();*/
                /* see library function "times" */
#define Too_Small_Time (SECS*HZ)
                /* Measurements should last at least about 2 seconds */
#endif
#ifdef TIME
extern long     time();
                /* see library function "time"  */
#define Too_Small_Time SECS
                /* Measurements should last at least 2 seconds */
#endif
#ifdef MSC_CLOCK
// extern clock_t clock();
#define Too_Small_Time (SECS*HZ)
#endif

long            Begin_Time,
                End_Time,
                User_Time;
#if 0   //add by KT
float           Microseconds,
                Dhrystones_Per_Second;
#else
long            Microseconds,
                Dhrystones_Per_Second;
#endif

/* end of variables for time measurement */

#ifdef NOMSG
extern int EarlyExit(void);

#ifdef PRINTSTATS
  volatile int * printstats;
#endif


// use direct call to Write0 (if available)
// this means that the stdio library components are not needed
void arm_puts(char *msg)
{
  #ifndef NOMSG
  #ifdef __arm
    __asm {
     call Write0
      mov r0,#0x4;
      mov r1,msg;
      swi 0x123456, {r0,r1};
    }
  #endif
  #else
  // use fputs so \n is not appended
  fputs(msg, stdout);
  #endif

}

#endif

int fp1_dhry_start(void)
/*****/

  /* main program, corresponds to procedures        */
  /* Main and Proc_0 in the Ada version             */
{
        One_Fifty       Int_1_Loc;
  REG   One_Fifty       Int_2_Loc;
        One_Fifty       Int_3_Loc;
  REG   char            Ch_Index;
        Enumeration     Enum_Loc;
        Str_30          Str_1_Loc;
        Str_30          Str_2_Loc;
  REG   long            Run_Index;
  REG   long            Number_Of_Runs = INIT_RUNS;

  /* Initializations */

#ifdef PRINTSTATS
  printstats=(int *)(0x03000000);
   (*printstats)=2;
#endif

  Next_Ptr_Glob = (Rec_Pointer) malloc (sizeof (Rec_Type));
  if(Next_Ptr_Glob == NULL)
  {   
    printk("Dhry Test: allocate memory for Rec_Pointer fail\n"); 
    return 0;   
  }           

  Ptr_Glob = (Rec_Pointer) malloc (sizeof (Rec_Type));
  if(Ptr_Glob == NULL)
  {   
    printk("Dhry Test: allocate memory for Rec_Pointer fail\n"); 
    return 0;   
  }   

  Ptr_Glob->Ptr_Comp                    = Next_Ptr_Glob;
  Ptr_Glob->Discr                       = Ident_1;
  Ptr_Glob->variant.var_1.Enum_Comp     = Ident_3;
  Ptr_Glob->variant.var_1.Int_Comp      = 40;
  strcpy (Ptr_Glob->variant.var_1.Str_Comp, 
          "DHRYSTONE PROGRAM, SOME STRING");
  strcpy (Str_1_Loc, "DHRYSTONE PROGRAM, 1'ST STRING");

#ifdef NOMSG
  /* NOMSG
   * If defined then we are generating vectors for the dhrystone loop
   * this is a very special case designed to generate as few vectors as
   * possible.  Number_Of_Runs is set to INIT_RUNS which defaults to 5.
   */
  arm_puts("go\n");
#else
# ifdef AUTO
  /* AUTO
   * automatically calculates dhrystone value starting with INIT_RUNS
   * iterations and doubling each time until SECS worth of execution
   * is exceeded
   */
  do {
    Number_Of_Runs *= 2;
    printf("Number of runs: %ld\n", Number_Of_Runs);
# else
  /* no defines
   * run full Dhrystone
   */
#if 0  //mark by KT   
  printf ("\n");
  printf ("Dhrystone Benchmark, Version 2.1 (Language: C)\n");
  printf ("\n");
  if (Reg)
    {
      printf ("Program compiled with 'register' attribute\n");
      printf ("\n");
    }
  else
    {
      printf ("Program compiled without 'register' attribute\n");
      printf ("\n");
    }

  printf ("Please give the number of runs through the benchmark: ");
  {
    long n;
    scanf ("%ld", &n);
    Number_Of_Runs = n;
  }
  
  printf ("Execution starts, %ld runs through Dhrystone\n", Number_Of_Runs);  
#else
    Number_Of_Runs = 100; 
#endif

# endif
#endif
  
  /* Was missing in published program. Without this statement,    */
  /* Arr_2_Glob [8][7] would have an undefined value.             */
  /* Warning: With 16-Bit processors and Number_Of_Runs > 32000,  */
  /* overflow may occur for this array element.                   */
  Arr_2_Glob [8][7] = 10;

  /***************/
  /* Start timer */
  /***************/
    
#ifdef NOMSG
  /* don't read a clock - just set start time to 0 */
  Begin_Time = 0;
#else
# ifdef TIMES
  times (&time_info);
  Begin_Time = (long) time_info.tms_utime;
# endif
  
# ifdef TIME
  Begin_Time = time ((long *) 0);
# endif
  
# ifdef MSC_CLOCK
  Begin_Time = clock();
# endif
#endif

  /* Simple bit of sentinel code to make spotting start of loop
     in EIS trace easier; will compile to something like:
       MOV      r0,#0xc9
       STR      r0,[r1,#0xd0]
     The next BL after the STR should correspond to the
     Proc_5() call.  If you grep the BL's address you should
     result in Number_Of_Runs matches.
  */
  Arr_2_Glob [1][2] = 201;
  
  for (Run_Index = 1; Run_Index <= Number_Of_Runs; ++Run_Index)
  {
/* This is herer to allow a GASP model to print out stats per loop
   It will artificially reduce peformance very slightly
   and may change register allocation
*/
#ifdef PRINTSTATS
   (*printstats)=3;
   (*printstats)=2;
#endif  

    Proc_5();
    Proc_4();
      /* Ch_1_Glob == 'A', Ch_2_Glob == 'B', Bool_Glob == true */
    Int_1_Loc = 2;
    Int_2_Loc = 3;
    strcpy (Str_2_Loc, "DHRYSTONE PROGRAM, 2'ND STRING");
    Enum_Loc = Ident_2;
    Bool_Glob = ! Func_2 (Str_1_Loc, Str_2_Loc);
      /* Bool_Glob == 1 */
    while (Int_1_Loc < Int_2_Loc)  /* loop body executed once */
    {
      Int_3_Loc = 5 * Int_1_Loc - Int_2_Loc;
	/* Int_3_Loc == 7 */
      Proc_7 (Int_1_Loc, Int_2_Loc, &Int_3_Loc);
	/* Int_3_Loc == 7 */
      Int_1_Loc += 1;
    } /* while */
      /* Int_1_Loc == 3, Int_2_Loc == 3, Int_3_Loc == 7 */
    Proc_8 (Arr_1_Glob, Arr_2_Glob, Int_1_Loc, Int_3_Loc);
      /* Int_Glob == 5 */
    Proc_1 (Ptr_Glob);
    for (Ch_Index = 'A'; Ch_Index <= Ch_2_Glob; ++Ch_Index)
			     /* loop body executed twice */
    {
      if (Enum_Loc == Func_1 (Ch_Index, 'C'))
          /* then, not executed */
        {
        Proc_6 (Ident_1, &Enum_Loc);
	strcpy (Str_2_Loc, "DHRYSTONE PROGRAM, 3'RD STRING");
	Int_2_Loc = (int) Run_Index;
        Int_Glob = (int) Run_Index;
        }
    }
      /* Int_1_Loc == 3, Int_2_Loc == 3, Int_3_Loc == 7 */
    Int_2_Loc = Int_2_Loc * Int_1_Loc;
    Int_1_Loc = Int_2_Loc / Int_3_Loc;
    Int_2_Loc = 7 * (Int_2_Loc - Int_3_Loc) - Int_1_Loc;
      /* Int_1_Loc == 1, Int_2_Loc == 13, Int_3_Loc == 7 */
    Proc_2 (&Int_1_Loc);
      /* Int_1_Loc == 5 */

  } /* loop "for Run_Index" */

  /**************/
  /* Stop timer */
  /**************/

  /* If NOMSG then print "ok" and call EarlyExit to stop
   * By doing it this way the compiler does not know we are stopping
   * and will compile all the code as per normal.
   * If you don't do this there is the danger of the extra optimisations
   * occuring.
   */
#ifdef NOMSG
  /* print simple completion message */
  arm_puts("ok\n");
  End_Time = 1;

#else
# ifdef TIMES
  times (&time_info);
  End_Time = (long) time_info.tms_utime;
# endif
# ifdef TIME
  End_Time = time ((long *) 0);
# endif
# ifdef MSC_CLOCK
  End_Time = clock();
# endif
#endif

  User_Time = End_Time - Begin_Time;
  
#ifdef AUTO
  /* repeat until time exceeds Too_Small_Time */
  } while (User_Time < Too_Small_Time);
#else
# ifndef NOMSG
#if 0 //add by KT
  /* report all the usual diagnostics */
  printf ("Execution ends\n"); 
  printf ("\n");  
  printf ("Final values of the variables used in the benchmark:\n");
  printf ("\n");
  printf ("Int_Glob:            %d\n", Int_Glob);
  printf ("        should be:   %d\n", 5);
  printf ("Bool_Glob:           %d\n", Bool_Glob);
  printf ("        should be:   %d\n", 1);
  printf ("Ch_1_Glob:           %c\n", Ch_1_Glob);
  printf ("        should be:   %c\n", 'A');
  printf ("Ch_2_Glob:           %c\n", Ch_2_Glob);
  printf ("        should be:   %c\n", 'B');
  printf ("Arr_1_Glob[8]:       %d\n", Arr_1_Glob[8]);
  printf ("        should be:   %d\n", 7);
  printf ("Arr_2_Glob[8][7]:    %d\n", Arr_2_Glob[8][7]);
  printf ("        should be:   Number_Of_Runs + 10\n");
  printf ("Ptr_Glob->\n");
  printf ("  Ptr_Comp:          %d\n", (int) Ptr_Glob->Ptr_Comp);
  printf ("        should be:   (implementation-dependent)\n");
  printf ("  Discr:             %d\n", Ptr_Glob->Discr);
  printf ("        should be:   %d\n", 0);
  printf ("  Enum_Comp:         %d\n", Ptr_Glob->variant.var_1.Enum_Comp);
  printf ("        should be:   %d\n", 2);
  printf ("  Int_Comp:          %d\n", Ptr_Glob->variant.var_1.Int_Comp);
  printf ("        should be:   %d\n", 17);
  printf ("  Str_Comp:          %s\n", Ptr_Glob->variant.var_1.Str_Comp);
  printf ("        should be:   DHRYSTONE PROGRAM, SOME STRING\n");
  printf ("Next_Ptr_Glob->\n");
  printf ("  Ptr_Comp:          %d\n", (int) Next_Ptr_Glob->Ptr_Comp);
  printf ("        should be:   (implementation-dependent), same as above\n");
  printf ("  Discr:             %d\n", Next_Ptr_Glob->Discr);
  printf ("        should be:   %d\n", 0);
  printf ("  Enum_Comp:         %d\n", Next_Ptr_Glob->variant.var_1.Enum_Comp);
  printf ("        should be:   %d\n", 1);
  printf ("  Int_Comp:          %d\n", Next_Ptr_Glob->variant.var_1.Int_Comp);
  printf ("        should be:   %d\n", 18);
  printf ("  Str_Comp:          %s\n", Next_Ptr_Glob->variant.var_1.Str_Comp);
  printf ("        should be:   DHRYSTONE PROGRAM, SOME STRING\n");
  printf ("Int_1_Loc:           %d\n", Int_1_Loc);
  printf ("        should be:   %d\n", 5);
  printf ("Int_2_Loc:           %d\n", Int_2_Loc);
  printf ("        should be:   %d\n", 13);
  printf ("Int_3_Loc:           %d\n", Int_3_Loc);
  printf ("        should be:   %d\n", 7);
  printf ("Enum_Loc:            %d\n", Enum_Loc);
  printf ("        should be:   %d\n", 1);
  printf ("Str_1_Loc:           %s\n", Str_1_Loc);
  printf ("        should be:   DHRYSTONE PROGRAM, 1'ST STRING\n");
  printf ("Str_2_Loc:           %s\n", Str_2_Loc);
  printf ("        should be:   DHRYSTONE PROGRAM, 2'ND STRING\n");
  printf ("\n");
#endif
# endif
#endif

  /* check the values, report failures via diag macros */
  /* NB checking the values ensures various optimisations don't take place */
  
#ifndef NOMSG
  /* full diagnostic printing using macros */
#define DIAG(a, b) if ((a) != (b)) { \
           printf( #a " was %x but should be %x\n", (unsigned int)a, (unsigned int)b); \
           fail = 1; \
        }
#define DIAG_S(a, b) if (strcmp(a, b) != 0) { \
           printf( #a " was '%s' but should be '%s'\n", a, b); \
           fail = 1; \
        }
#else
  /* no diagnostic printing for reduced program size */
#define   DIAG(a, b) if ((a) != (b)) fail = 1
#define DIAG_S(a, b) if (strcmp(a, b) != 0) fail = 1
#endif
  
  {
    int fail = 0;

    DIAG  ( Int_Glob, 5);
    DIAG  (Bool_Glob, 1);
    DIAG  (Ch_1_Glob, 'A');
    DIAG  (Ch_2_Glob, 'B');
    DIAG  (Arr_1_Glob[8], 7);
    DIAG  (Arr_2_Glob[8][7], Number_Of_Runs + 10);
    DIAG  (Ptr_Glob->Ptr_Comp, Next_Ptr_Glob);
    DIAG  (Ptr_Glob->Discr , 0);
    DIAG  (Ptr_Glob->variant.var_1.Enum_Comp, 2);
    DIAG  (Ptr_Glob->variant.var_1.Int_Comp, 17);
    DIAG_S(Ptr_Glob->variant.var_1.Str_Comp, "DHRYSTONE PROGRAM, SOME STRING");
    DIAG  (Next_Ptr_Glob->Ptr_Comp, Next_Ptr_Glob);
    DIAG  (Next_Ptr_Glob->Discr, 0);
    DIAG  (Next_Ptr_Glob->variant.var_1.Enum_Comp, 1);
    DIAG  (Next_Ptr_Glob->variant.var_1.Int_Comp, 18);
    DIAG_S(Next_Ptr_Glob->variant.var_1.Str_Comp, "DHRYSTONE PROGRAM, SOME STRING");
    DIAG  (Int_1_Loc, 5);
    DIAG  (Int_2_Loc, 13);
    DIAG  (Int_3_Loc, 7);
    DIAG  ( Enum_Loc, 1);
    DIAG_S(Str_1_Loc, "DHRYSTONE PROGRAM, 1'ST STRING");
    DIAG_S(Str_2_Loc, "DHRYSTONE PROGRAM, 2'ND STRING");

    //add by KT
    if(Next_Ptr_Glob != NULL)
        vfree(Next_Ptr_Glob);
    if(Ptr_Glob != NULL)
        vfree(Ptr_Glob);

    if (fail) {
#ifdef NOMSG
      // omit printf so that all the stdio library components aren't needed
      arm_puts("** TEST FAILED **\n");
#else
      printf("** TEST FAILED **\n");
#endif
#if 0
      exit(1);
#else 
      return 0;
#endif
    }
  }
  
#ifdef NOMSG
  // omit floating point calculations so that all the math library components aren't needed
  arm_puts("** TEST PASSED OK **\n\002");
#else

#if 0  //add by KT
# ifdef TIME
  Microseconds = (float) User_Time * Mic_secs_Per_Second / (float) Number_Of_Runs;
  Dhrystones_Per_Second = (float) Number_Of_Runs / (float) User_Time;
# else
  Microseconds = (float) User_Time * Mic_secs_Per_Second / ((float) HZ * ((float) Number_Of_Runs));
  Dhrystones_Per_Second = ((float) HZ * (float) Number_Of_Runs) / (float) User_Time;
# endif

  printf("Execution time %lds.\n", User_Time/HZ);
  printf("Microseconds per loop: %6.4f\n", Microseconds);
  printf("Dhrystones per Second: %6.1f\n", Dhrystones_Per_Second);
  printf("Dhrystone  MIPS:       %6.3f\n", Dhrystones_Per_Second/1757.0);

  // Stop val_report thinking the test has abandoned
  printf("** TEST PASSED OK **\n");
#endif
  
  return 1; //add by KT
#endif
}


void Proc_1 (REG Rec_Pointer Ptr_Val_Par)
/******************/
    /* executed once */
{
  REG Rec_Pointer Next_Record = Ptr_Val_Par->Ptr_Comp;  
                                        /* == Ptr_Glob_Next */
  /* Local variable, initialized with Ptr_Val_Par->Ptr_Comp,    */
  /* corresponds to "rename" in Ada, "with" in Pascal           */

  structassign (*Ptr_Val_Par->Ptr_Comp, *Ptr_Glob); 
  Ptr_Val_Par->variant.var_1.Int_Comp = 5;
  Next_Record->variant.var_1.Int_Comp 
        = Ptr_Val_Par->variant.var_1.Int_Comp;
  Next_Record->Ptr_Comp = Ptr_Val_Par->Ptr_Comp;
  Proc_3 (&Next_Record->Ptr_Comp);
    /* Ptr_Val_Par->Ptr_Comp->Ptr_Comp 
                        == Ptr_Glob->Ptr_Comp */
  if (Next_Record->Discr == Ident_1)
    /* then, executed */
  {
    Next_Record->variant.var_1.Int_Comp = 6;
    Proc_6 (Ptr_Val_Par->variant.var_1.Enum_Comp,
           &Next_Record->variant.var_1.Enum_Comp);
    Next_Record->Ptr_Comp = Ptr_Glob->Ptr_Comp;
    Proc_7 (Next_Record->variant.var_1.Int_Comp, 10, 
           &Next_Record->variant.var_1.Int_Comp);
  }
  else /* not executed */
    structassign (*Ptr_Val_Par, *Ptr_Val_Par->Ptr_Comp);
} /* Proc_1 */


void Proc_2 (One_Fifty *Int_Par_Ref)
/******************/
    /* executed once */
    /* *Int_Par_Ref == 1, becomes 4 */
{
  One_Fifty  Int_Loc;  
  Enumeration   Enum_Loc;

  Int_Loc = *Int_Par_Ref + 10;
  do /* executed once */
    if (Ch_1_Glob == 'A')
      /* then, executed */
    {
      Int_Loc -= 1;
      *Int_Par_Ref = Int_Loc - Int_Glob;
      Enum_Loc = Ident_1;
    } /* if */
  while (Enum_Loc != Ident_1); /* true */
} /* Proc_2 */


void Proc_3 (Rec_Pointer *Ptr_Ref_Par)
/******************/
    /* executed once */
    /* Ptr_Ref_Par becomes Ptr_Glob */
{
  if (Ptr_Glob != Null)
    /* then, executed */
    *Ptr_Ref_Par = Ptr_Glob->Ptr_Comp;
  Proc_7 (10, Int_Glob, &Ptr_Glob->variant.var_1.Int_Comp);
} /* Proc_3 */


void Proc_4 () /* without parameters */
/*******/
    /* executed once */
{
  Boolean Bool_Loc;

  Bool_Loc = Ch_1_Glob == 'A';
  Bool_Glob = Bool_Loc | Bool_Glob;
  Ch_2_Glob = 'B';
} /* Proc_4 */


void Proc_5 () /* without parameters */
/*******/
    /* executed once */
{
  Ch_1_Glob = 'A';
  Bool_Glob = false;
} /* Proc_5 */


	/* Procedure for the assignment of structures,          */
        /* if the C compiler doesn't support this feature       */
#ifdef  NOSTRUCTASSIGN
void memcpy (register char *d, register char *s, register int l)
{
        while (l--) *d++ = *s++;
}
#endif


