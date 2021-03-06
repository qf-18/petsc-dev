
/*
    Defines the operations for the X PetscDraw implementation.
*/

#include <petsc-private/drawimpl.h>         /*I  "petscsys.h" I*/

typedef struct {
  char      *filename;
  FILE      *fd;
  PetscBool written;  /* something has been written to the current frame */
} PetscDraw_TikZ;

#define TikZ_BEGIN_DOCUMENT  "\\documentclass{beamer}\n\n\
\\usepackage{tikz}\n\
\\usepackage{pgflibraryshapes}\n\
\\usetikzlibrary{backgrounds}\n\
\\usetikzlibrary{arrows}\n\
\\newenvironment{changemargin}[2]{%%\n\
  \\begin{list}{}{%%\n\
    \\setlength{\\topsep}{0pt}%%\n\
    \\setlength{\\leftmargin}{#1}%%\n\
    \\setlength{\\rightmargin}{#2}%%\n\
    \\setlength{\\listparindent}{\\parindent}%%\n\
    \\setlength{\\itemindent}{\\parindent}%%\n\
    \\setlength{\\parsep}{\\parskip}%%\n\
  }%%\n\
  \\item[]}{\\end{list}}\n\n\
\\begin{document}\n"

#define TikZ_BEGIN_FRAME "\\begin{frame}{}\n\
\\begin{changemargin}{-1cm}{0cm}\n\
\\begin{center}\n\
\\begin{tikzpicture}[scale = 10.00,font=\\fontsize{8}{8}\\selectfont]\n"

#define TikZ_END_FRAME "\\end{tikzpicture}\n\
\\end{center}\n\
\\end{changemargin}\n\
\\end{frame}\n"

#define TikZ_END_DOCUMENT  "\\end{document}\n"

#undef __FUNCT__
#define __FUNCT__ "PetscDrawDestroy_TikZ"
PetscErrorCode  PetscDrawDestroy_TikZ(PetscDraw draw)
{
  PetscDraw_TikZ *win = (PetscDraw_TikZ*)draw->data;
  PetscErrorCode ierr;

  PetscFunctionBegin;
  ierr = PetscFPrintf(((PetscObject)draw)->comm,win->fd,TikZ_END_FRAME);CHKERRQ(ierr);
  ierr = PetscFPrintf(((PetscObject)draw)->comm,win->fd,TikZ_END_DOCUMENT);CHKERRQ(ierr);
  ierr = PetscFClose(((PetscObject)draw)->comm,win->fd);CHKERRQ(ierr);
  ierr = PetscFree(win->filename);CHKERRQ(ierr);
  ierr = PetscFree(win);CHKERRQ(ierr);
  PetscFunctionReturn(0);
}

static const char *TikZColors[] = { "white",  "black", "red",  "green", "cyan",   "blue", "magenta", 0, 0, "orange",
                                    "violet", "brown", "pink", 0,       "yellow", 0};

PETSC_STATIC_INLINE const char * TikZColorMap(int cl)
{
  return((cl < 16) ? ( TikZColors[cl] ? TikZColors[cl] : "black") : "black");
}

/*
     These macros transform from the users coordinates to the (0,0) -> (1,1) coordinate system
*/
#define XTRANS(draw,x)  (double)(((draw)->port_xl + (((x - (draw)->coor_xl)*((draw)->port_xr - (draw)->port_xl))/((draw)->coor_xr - (draw)->coor_xl))))
#define YTRANS(draw,y)  (double)(((draw)->port_yl + (((y - (draw)->coor_yl)*((draw)->port_yr - (draw)->port_yl))/((draw)->coor_yr - (draw)->coor_yl))))

#undef __FUNCT__
#define __FUNCT__ "PetscDrawClear_TikZ"
PetscErrorCode PetscDrawClear_TikZ(PetscDraw draw)
{
  PetscDraw_TikZ *win = (PetscDraw_TikZ*)draw->data;
  PetscErrorCode ierr;

  PetscFunctionBegin;
  /* often PETSc generates unneeded clears, we want avoid creating empy pictures for them */
  if (!win->written) PetscFunctionReturn(0);
  ierr = PetscFPrintf(((PetscObject)draw)->comm,win->fd,TikZ_END_FRAME);CHKERRQ(ierr);
  ierr = PetscFPrintf(((PetscObject)draw)->comm,win->fd,TikZ_BEGIN_FRAME);CHKERRQ(ierr);
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "PetscDrawLine_TikZ"
PetscErrorCode PetscDrawLine_TikZ(PetscDraw draw,PetscReal xl,PetscReal yl,PetscReal xr,PetscReal yr,int cl)
{
  PetscDraw_TikZ *win = (PetscDraw_TikZ*)draw->data;
  PetscErrorCode ierr;

  PetscFunctionBegin;
  win->written = PETSC_TRUE;
  ierr = PetscFPrintf(((PetscObject)draw)->comm,win->fd,"\\draw [%s] (%g,%g) --(%g,%g);\n",TikZColorMap(cl),XTRANS(draw,xl),YTRANS(draw,yl),XTRANS(draw,xr),YTRANS(draw,yr));CHKERRQ(ierr);
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "PetscDrawString_TikZ"
PetscErrorCode PetscDrawString_TikZ(PetscDraw draw,PetscReal xl,PetscReal yl,int cl,const char text[])
{
  PetscDraw_TikZ *win = (PetscDraw_TikZ*)draw->data;
  PetscErrorCode ierr;

  PetscFunctionBegin;
  win->written = PETSC_TRUE;
  ierr = PetscFPrintf(((PetscObject)draw)->comm,win->fd,"\\node [above right, %s] at (%g,%g) {%s};\n",TikZColorMap(cl),XTRANS(draw,xl),YTRANS(draw,yl),text);CHKERRQ(ierr);
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "PetscDrawBoxedString_TikZ"
/*
    Does not handle multiline strings correctly
*/
PetscErrorCode PetscDrawBoxedString_TikZ(PetscDraw draw,PetscReal xl,PetscReal yl,int cl,int ct,const char text[],PetscReal *w,PetscReal *h)
{
  PetscDraw_TikZ *win = (PetscDraw_TikZ*)draw->data;
  PetscErrorCode ierr;
  size_t         len;

  PetscFunctionBegin;
  win->written = PETSC_TRUE;
  ierr = PetscFPrintf(((PetscObject)draw)->comm,win->fd,"\\draw (%g,%g) node [rectangle, draw, align=center, inner sep=1ex] {%s};\n",XTRANS(draw,xl),YTRANS(draw,yl),text);CHKERRQ(ierr);

  /* make up totally bogus height and width of box */
  ierr = PetscStrlen(text,&len);CHKERRQ(ierr);
  if (w) *w = .07*len;
  if (h) *h = .07;
  PetscFunctionReturn(0);
}

static struct _PetscDrawOps DvOps = { 0,
                                      0,
                                      PetscDrawLine_TikZ,
                                      0,
                                      0,
                                      0,
                                      0,
                                      PetscDrawString_TikZ,
                                      0,
                                      0,
                                      0,
                                      0,
                                      PetscDrawClear_TikZ,
                                      0,
                                      0,
                                      0,
                                      0,
                                      0,
                                      0,
                                      0,
                                      0,
                                      0,
                                      0,
                                      0,
                                      0,
                                      0,
                                      PetscDrawDestroy_TikZ,
                                      0,
                                      0,
                                      0,
                                      0,
                                      0,
                                      0,
                                      0,
                                      0,
                                      0,
                                      0,
                                      PetscDrawBoxedString_TikZ};




EXTERN_C_BEGIN
#undef __FUNCT__
#define __FUNCT__ "PetscDrawCreate_TikZ"
PetscErrorCode  PetscDrawCreate_TikZ(PetscDraw draw)
{
  PetscDraw_TikZ *win;
  PetscErrorCode ierr;

  PetscFunctionBegin;
  ierr = PetscMemcpy(draw->ops,&DvOps,sizeof(DvOps));CHKERRQ(ierr);
  ierr = PetscNew(PetscDraw_TikZ,&win);CHKERRQ(ierr);
  ierr = PetscLogObjectMemory(draw,sizeof(PetscDraw_TikZ));CHKERRQ(ierr);
  draw->data = (void*) win;

  if (draw->title) {
    ierr = PetscStrallocpy(draw->title,&win->filename);CHKERRQ(ierr);
  } else {
    const char *fname;
    ierr = PetscObjectGetName((PetscObject)draw,&fname);CHKERRQ(ierr);
    ierr = PetscStrallocpy(fname,&win->filename);CHKERRQ(ierr);
  }
  ierr = PetscFOpen(((PetscObject)draw)->comm,win->filename,"w",&win->fd);CHKERRQ(ierr);
  ierr = PetscFPrintf(((PetscObject)draw)->comm,win->fd,TikZ_BEGIN_DOCUMENT);CHKERRQ(ierr);
  ierr = PetscFPrintf(((PetscObject)draw)->comm,win->fd,TikZ_BEGIN_FRAME);CHKERRQ(ierr);
  win->written = PETSC_FALSE;
  PetscFunctionReturn(0);
}
EXTERN_C_END









