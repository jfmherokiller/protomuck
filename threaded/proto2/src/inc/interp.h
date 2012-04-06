#ifndef __INCL_INTERP_H
#define __INCL_INTERP_H


/* Stuff the interpreter needs. */
/* Some icky machine/compiler #defines. --jim */
#ifdef MIPS
typedef char * voidptr;

#define MIPSCAST (char *)
#else
typedef void * voidptr;

#define MIPSCAST
#endif

#define UPCASE(x) (toupper(x))
#define DOWNCASE(x) (tolower(x))

#define DoNullInd(x) ((x) ? (x) -> data : "")
  
extern void do_abort_silent(struct frame *fr);

extern void RCLEAR(struct inst *oper, const char *file, int line);

#define PSafe (OkObj(player) ? player : OWNER(program))

#define CLEAR(oper) RCLEAR(oper, __FILE__, __LINE__)
extern void push (struct inst *stack, int *top, int type, voidptr res);
extern int valid_object(struct inst *oper);
  
extern struct localvars *localvars_get(struct frame *fr, dbref prog);
extern void localvar_dupall(struct frame *fr, struct frame *oldfr);
extern struct inst *scopedvar_get(struct frame *fr, int level, int varnum);
extern const char* scopedvar_getname(struct frame *fr, int level, int varnum);
extern const char* scopedvar_getname_byinst(struct inst *pc, int varnum);
extern int scopedvar_getnum(struct frame *fr, int level, const char* varname);
extern void scopedvar_dupall(struct frame *fr, struct frame *oldfr);
extern int logical_false(struct inst *p);

extern void copyinst(struct inst *from, struct inst *to);

/* From p_socket.c */
extern void remove_socket_from_queue(struct muf_socket *oldSock);
  
#define abort_loop(S, C1, C2) \
{ \
  do_abort_loop(player, program, (S), fr, pc, atop, stop, (C1), (C2)); \
  if (fr && fr->trys.top) \
      break; \
  else { \
      mutex_unlock(fr->mutx); \
	  if (err) *err = 1; \
      return 0; \
  } \
}
  
#define abort_loop_hard(S, C1, C2) \
{ \
	int tmp = 0; \
	if (fr) { tmp = fr->trys.top; fr->trys.top = 0; } \
	do_abort_loop(player, program, (S), fr, pc, atop, stop, (C1), (C2)); \
	if (fr) fr->trys.top = tmp; \
	if (fr) mutex_unlock(fr->mutx); \
	if (err) *err = 1; \
	return 0; \
}

extern void do_abort_loop(dbref player, dbref program, const char *msg,
                          struct frame *fr, struct inst *pc,
                          int atop, int stop, struct inst *clinst1,
                          struct inst *clinst2);
  
extern void interp_err(struct frame *fr, dbref player, dbref program, struct inst *pc,
                       struct inst *arg, int atop, dbref origprog,
                       const char *msg1, const char *msg2, int pid);
  
extern void push (struct inst *stack, int *top, int type, voidptr res);
  
extern int valid_player(struct inst *oper);
  
extern int valid_object(struct inst *oper);
  
extern int is_home(struct inst *oper);
  
extern int permissions(int mlev, dbref player, dbref thing);
extern int newpermissions(int mlev, dbref player, dbref thing, bool true_c);
  
extern int arith_type(struct inst *op1, struct inst *op2);

extern void interp_set_depth(struct frame * fr);
  
#define CHECKOP_READONLY(N) \
{ \
    if (*top < N) \
        abort_interp("Stack underflow."); \
}

#define CHECKOP(N) \
{ \
    CHECKOP_READONLY(N); \
    if (fr->trys.top && *top - fr->trys.st->depth < N) {\
        abort_interp("Stack protection fault."); \
    }\
}

#define POP() (arg + --(*top))
  
#define abort_interp(C) \
{ \
  do_abort_interp(player, (C), pc, arg, *top, fr, program, __FILE__, __LINE__); \
  return; \
}
extern void do_abort_interp(dbref player, const char *msg, struct inst *pc,
     struct inst *arg, int atop, struct frame *fr, 
     dbref program, const char *file, int line);

#define PRIM_PROTOTYPE dbref player, dbref program, int mlev, \
                       struct inst *pc, struct inst *arg, int *top, \
                       struct frame *fr, struct inst *oper

struct prim_list {
    const char *name;  /* The prim's name */
    int mlev;          /* Minimum mlevel to use this prim */
    int nargs;         /* Number of args input */
    void (*func)(dbref player, dbref program, int mlev, \
                       struct inst *pc, struct inst *arg, int *top, \
                       struct frame *fr, struct inst *oper);    /* The function to call */
};

extern struct prim_list primlist[];

#define Min(x,y) ((x < y) ? x : y)
#define ProgMLevel(x) (find_mlev(x, fr, fr->caller.top))

#define ProgUID find_uid(player, fr, fr->caller.top, program)
extern int find_mlev(dbref prog, struct frame *fr, int st);
extern dbref find_uid(dbref player, struct frame *fr, int st, dbref program);

#define CHECKREMOTE(x) if ((mlev < 2) && (getloc(x) != player) &&  \
                           (getloc(x) != getloc(player)) && \
                           ((x) != getloc(player)) && ((x) != player) \
			   && !controls(ProgUID, x)) \
                 abort_interp("Mucker Level 2 required to get remote info.");

#define PushObject(x)   push(arg, top, PROG_OBJECT, MIPSCAST &x)
#define PushInt(x)      push(arg, top, PROG_INTEGER, MIPSCAST &x)
#define PushFloat(x)    push(arg, top, PROG_FLOAT, MIPSCAST &x)
#define PushLock(x)     push(arg, top, PROG_LOCK, MIPSCAST copy_bool(x))
#define PushTrueLock(x) push(arg, top, PROG_LOCK, MIPSCAST TRUE_BOOLEXP)

#define PushMark()      push(arg, top, PROG_MARK, MIPSCAST 0)
#define PushStrRaw(x)   push(arg, top, PROG_STRING, MIPSCAST x)
#define PushString(x)   PushStrRaw(alloc_prog_string(x))
#define PushNullStr     PushStrRaw(0)

#define PushArrayRaw(x) push(arg, top, PROG_ARRAY, MIPSCAST x)
#define PushNullArray   PushArrayRaw(0)
#define PushInst(x) copyinst(x, &arg[((*top)++)])

#define CHECKOFLOW(x) if((*top + (x - 1)) >= STACK_SIZE) \
			  abort_interp("Stack Overflow!");

#define SORTTYPE_CASEINSENS     0x1 
#define SORTTYPE_DESCENDING     0x2 

#define SORTTYPE_CASE_ASCEND    0
#define SORTTYPE_NOCASE_ASCEND  (SORTTYPE_CASEINSENS) 
#define SORTTYPE_CASE_DESCEND   (SORTTYPE_DESCENDING) 
#define SORTTYPE_NOCASE_DESCEND (SORTTYPE_CASEINSENS | SORTTYPE_DESCENDING) 

#define SORTTYPE_NATURAL_CASE_ASCEND    (SORTTYPE_NATURAL)
#define SORTTYPE_NATURAL_NOCASE_ASCEND  (SORTTYPE_NATURAL | SORTTYPE_CASEINSENS)
#define SORTTYPE_NATURAL_CASE_DESCEND   (SORTTYPE_NATURAL | SORTTYPE_DESCENDING)
#define SORTTYPE_NATURAL_NOCASE_DESCEND (SORTTYPE_NATURAL | SORTTYPE_DESCENDING | SORTTYPE_CASEINSENS)

#define SORTTYPE_SHUFFLE        0x4
#define SORTTYPE_NATURAL        0x8

/* extern int    nargs; */ /* DO NOT TOUCH THIS VARIABLE */ /* I'm touching it!  This is part of the frame now.  -hinoserm */

#ifndef PROTOMUCK_MODULE
# include "p_connects.h"
# include "p_db.h"
# include "p_math.h"
# include "p_misc.h"
# include "p_props.h"
# include "p_stack.h"
# include "p_strings.h"
# include "p_mcp.h"
# include "p_float.h"
# include "p_error.h"
# include "p_file.h"
# include "p_array.h"
# include "p_socket.h"
# include "p_system.h"
# include "p_mysql.h"
# include "p_muf.h"
# include "p_regex.h"
# include "p_http.h" /* hinoserm */
#endif

#endif
