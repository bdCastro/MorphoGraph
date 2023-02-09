/* $Id: mcimage.c,v 1.25 2007/07/10 08:49:29 michel Exp $ */
/* 
   Librairie mcimage : 

   fonctions pour les entrees/sorties fichiers et l'allocation de structures
   image en memoire.

   Michel Couprie 1996

   Avertissement: la lecture des fichiers PGM en 65535 ndg (ascii) provoque un sous-echantillonage
   a 256 ndg. A MODIFIER.

   Update septembre 2000 : lecture de fichiers BMP truecolor
   Update octobre 2000 : ecriture de fichiers BMP truecolor
   Update janvier 2002 : lecture d'elements structurants (readse)
   Update mars 2002 : nettoie la gestion des noms
   Update decembre 2002 : type FLOAT
   Update decembre 2002 : convertgen
   Update avril 2003 : convertfloat
   Update janvier 2006 : adoption des nouveaux "magic numbers" pour
                         les formats byte 3d, idem 2d (P2 et P5)
			 P7 (raw 3d) est conserv� pour la compatibilit�
*/

#include <stdio.h>
#include <stdint.h>
#include <sys/types.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "mcutil.h"
#include "mcimage.h"
#include "mccodimage.h"

#define BUFFERSIZE 10000

#define UNIXIO

/*
#define VERBOSE
*/

/* ==================================== */
struct xvimage *allocimage(
  char * name,
  int rs,   /* row size */
  int cs,   /* col size */
  int d,    /* depth */
  int t)    /* data type */
/* ==================================== */
#undef F_NAME
#define F_NAME "allocimage"
{
  int N = rs * cs * d;             /* taille image */
  struct xvimage *g;
  int ts;                          /* type size */

  switch (t)
  {
    case VFF_TYP_BIT:      ts = 1; break;
    case VFF_TYP_1_BYTE:   ts = 1; break;
    case VFF_TYP_2_BYTE:   ts = 2; break;
    case VFF_TYP_4_BYTE:   ts = 4; break;
    case VFF_TYP_FLOAT:    ts = sizeof(float); break;
    case VFF_TYP_DOUBLE:   ts = sizeof(double); break;
    default: fprintf(stderr,"%s : bad data type %d\n", F_NAME, t);
             return NULL;
  } /* switch (t) */

  g = (struct xvimage *)malloc(sizeof(struct xvimage));
  if (g == NULL)
  {   
    fprintf(stderr,"%s : malloc failed (%d bytes)\n", F_NAME, sizeof(struct xvimage));
    return NULL;
  }

  g->image_data = (void *)calloc(1, N * ts);
  if (g == NULL)
  {   
    fprintf(stderr,"%s : calloc failed (%d bytes)\n", F_NAME, N * ts);
    return NULL;
  }

  if (name != NULL)
  {
    g->name = (char *)calloc(1,strlen(name)+1);
    if (g->name == NULL)
    {   fprintf(stderr,"%s : malloc failed for name\n", F_NAME);
        return NULL;
    }
    strcpy((char *)(g->name), name);
  }
  else
    g->name = NULL;

  rowsize(g) = rs;
  colsize(g) = cs;
  depth(g) = d;
  datatype(g) = t;
  g->xdim = g->ydim = g->zdim = 0.0;

  return g;
} /* allocimage() */

/* ==================================== */
void razimage(struct xvimage *f)
/* ==================================== */
#undef F_NAME
#define F_NAME "razimage"
{
  int rs = rowsize(f);         /* taille ligne */
  int cs = colsize(f);         /* taille colonne */
  int ds = depth(f);           /* nb plans */
  int N = rs * cs * ds;        /* taille image */
  int ts; 
  uint8_t *F = UCHARDATA(f);

  switch(datatype(f))
  {
    case VFF_TYP_BIT:      ts = 1; break;
    case VFF_TYP_1_BYTE:   ts = 1; break;
    case VFF_TYP_2_BYTE:   ts = 2; break;
    case VFF_TYP_4_BYTE:   ts = 4; break;
    case VFF_TYP_FLOAT:    ts = sizeof(float); break;
    case VFF_TYP_DOUBLE:   ts = sizeof(double); break;
    default: fprintf(stderr,"%s : bad data type %d\n", F_NAME, datatype(f));
             return;
  } /* switch (t) */
  memset(F, 0, N*ts);
} /* razimage() */

/* ==================================== */
struct xvimage *allocheader(
  char * name,
  int rs,   /* row size */
  int cs,   /* col size */
  int d,    /* depth */
  int t)    /* data type */
/* ==================================== */
#undef F_NAME
#define F_NAME "allocheader"
{
  struct xvimage *g;

  g = (struct xvimage *)malloc(sizeof(struct xvimage));
  if (g == NULL)
  {   fprintf(stderr,"%s : malloc failed\n", F_NAME);
      return NULL;
  }
  if (name != NULL)
  {
    g->name = (char *)calloc(1,strlen(name)+1);
    if (g->name == NULL)
    {   fprintf(stderr,"%s : malloc failed for name\n", F_NAME);
        return NULL;
    }
    strcpy((char *)(g->name), name);
  }
  else
    g->name = NULL;

  g->image_data = NULL;
  rowsize(g) = rs;
  colsize(g) = cs;
  depth(g) = d;
  datatype(g) = t;
  g->xdim = g->ydim = g->zdim = 0.0;

  return g;
} /* allocheader() */

/* ==================================== */
int showheader(char * name)
/* ==================================== */
#undef F_NAME
#define F_NAME "showheader"
{
  char buffer[BUFFERSIZE];
  FILE *fd = NULL;
  int rs, cs, d, c;
  char *read;
#ifdef UNIXIO
  fd = fopen(name,"r");
#endif
#ifdef DOSIO
  fd = fopen(name,"rb");
#endif
  if (!fd)
  {
    fprintf(stderr, "%s: file not found: %s\n", F_NAME, name);
    return 0;
  }

  read = fgets(buffer, BUFFERSIZE, fd);
  if (!read)
  {
    fprintf(stderr, "%s: fgets returned without reading\n", F_NAME);
    return 0;
  }
  if (buffer[0] != 'P')
  {   
    fprintf(stderr,"%s : invalid image format : %c%c\n", F_NAME, buffer[0], buffer[1]);
    return 0;
  }
  switch (buffer[1])
  {
    case '2': printf("type : P%c (ascii byte)\n", buffer[1]); break;
    case '3': printf("type : P%c (ascii byte rgb)\n", buffer[1]); break;
    case '5': printf("type : P%c (raw byte)\n", buffer[1]); break;
    case '6': printf("type : P%c (raw byte rgb)\n", buffer[1]); break;
    case '7': printf("type : P%c (raw byte 3d - ext. MC [OBSOLETE - USE P5])\n", buffer[1]); break;
    case '8': printf("type : P%c (raw int - ext. MC)\n", buffer[1]); break;
    case '9': printf("type : P%c (raw float - ext. MC)\n", buffer[1]); break;
    case 'A': printf("type : P%c (ascii float - ext. LN)\n", buffer[1]); break;
    case 'B': printf("type : P%c (ascii int - ext. MC)\n", buffer[1]); break;
              break;
    default:
      fprintf(stderr,"%s : invalid image format : P%c\n", F_NAME, buffer[1]);
      return 0;
  } /* switch */

  do 
  {
    read = fgets(buffer, BUFFERSIZE, fd);
    if (!read)
    {
      fprintf(stderr, "%s: fgets returned without reading\n", F_NAME);
      return 0;
    }
    if (buffer[0] == '#') /* commentaire */
      printf("comment : %s", buffer+1);
  } while (!isdigit(buffer[0]));

  c = sscanf(buffer, "%d %d %d", &rs, &cs, &d);
  if (c == 2) printf("size : rowsize = %d ; colsize = %d\n", rs, cs);
  else if (c == 3) printf("size : rowsize = %d ; colsize = %d ; depth = %d\n", rs, cs, d);
  else
  {   
    fprintf(stderr,"%s : invalid image format : cannot find image size\n", F_NAME);
    return 0;
  }

  fclose(fd);
  return 1;
} // showheader() 

/* ==================================== */
void freeimage(struct xvimage *image)
/* ==================================== */
{
  if (image->name != NULL) free(image->name); 
  free(image->image_data);
  free(image);
}

/* ==================================== */
struct xvimage *copyimage(struct xvimage *f)
/* ==================================== */
#undef F_NAME
#define F_NAME "copyimage"
{
  int rs = rowsize(f);         /* taille ligne */
  int cs = colsize(f);         /* taille colonne */
  int ds = depth(f);           /* nb plans */
  int N = rs * cs * ds;        /* taille image */
  int type = datatype(f);
  struct xvimage *g;

  g = allocimage(NULL, rs, cs, ds, type);
  if (g == NULL)
  {
    fprintf(stderr,"%s : allocimage failed\n", F_NAME);
    return NULL;
  }

  switch(type)
  {
    case VFF_TYP_1_BYTE: memcpy(g->image_data, f->image_data, (N*sizeof(int8_t))); break;
    case VFF_TYP_4_BYTE: memcpy(g->image_data, f->image_data, (N*sizeof(int))); break;
    case VFF_TYP_FLOAT:  memcpy(g->image_data, f->image_data, (N*sizeof(float))); break;
    case VFF_TYP_DOUBLE: memcpy(g->image_data, f->image_data, (N*sizeof(double))); break;
    default:
      fprintf(stderr,"%s : bad data type %d\n", F_NAME, type);
      return NULL;
  } /* switch(f->datatype) */

  if (f->name != NULL)
  {
    g->name = (char *)calloc(1,strlen(f->name) + 1);
    if (g->name == NULL)
    {   fprintf(stderr,"%s : malloc failed for name\n", F_NAME);
        return NULL;
    }
    strcpy(g->name, f->name);
  }
  return g;
} // copyimage()

/* ==================================== */
int copy2image(struct xvimage *dest, struct xvimage *source)
/* ==================================== */
#undef F_NAME
#define F_NAME "copy2image"
{
  int rs = rowsize(source);         /* taille ligne */
  int cs = colsize(source);         /* taille colonne */
  int ds = depth(source);           /* nb plans */
  int N = rs * cs * ds;             /* taille image */
  if ((rowsize(dest) != rs) || (colsize(dest) != cs) || (depth(dest) != ds))
  {
    fprintf(stderr, "%s: incompatible image sizes\n", F_NAME);
    return 0;
  }
  if (datatype(dest) != datatype(source))
  {
    fprintf(stderr, "%s: incompatible image types\n", F_NAME);
    return 0;
  }
  switch(datatype(source))
  {
    case VFF_TYP_1_BYTE:
      {
        uint8_t *S = UCHARDATA(source);
        uint8_t *D = UCHARDATA(dest);
        memcpy(D, S, N*sizeof(char));
        break;
      }
    case VFF_TYP_4_BYTE:
      {
        unsigned int *S = ULONGDATA(source);
        unsigned int *D = ULONGDATA(dest);
        memcpy(D, S, N*sizeof(int));
        break;
      }
    case VFF_TYP_FLOAT:
      {
        float *S = FLOATDATA(source);
        float *D = FLOATDATA(dest);
        memcpy(D, S, N*sizeof(float));
        break;
      }
    case VFF_TYP_DOUBLE:
      {
        double *S = DOUBLEDATA(source);
        double *D = DOUBLEDATA(dest);
        memcpy(D, S, N*sizeof(double));
        break;
      }
    default:
      fprintf(stderr,"%s : bad data type %d\n", F_NAME, datatype(source));
      return 0;
  } /* switch(f->datatype) */
  return 1;
} // copy2image()

/* ==================================== */
int equalimages(struct xvimage *im1, struct xvimage *im2)
/* ==================================== */
// checks whether the two images are equal
#undef F_NAME
#define F_NAME "equalimages"
{
  int rs = rowsize(im1);         /* taille ligne */
  int cs = colsize(im1);         /* taille colonne */
  int ds = depth(im1);           /* nb plans */
  int N = rs * cs * ds;             /* taille image */
  if ((rowsize(im2) != rs) || (colsize(im2) != cs) || (depth(im2) != ds)) return 0;
  if (datatype(im2) != datatype(im1)) return 0;
  switch(datatype(im1))
  {
    case VFF_TYP_1_BYTE:
      {
        uint8_t *I1 = UCHARDATA(im1);
        uint8_t *I2 = UCHARDATA(im2);
        if (memcmp(I1, I2, N*sizeof(char)) != 0) return 0;
        break;
      }
    case VFF_TYP_4_BYTE:
      {
        unsigned int *I1 = ULONGDATA(im1);
        unsigned int *I2 = ULONGDATA(im2);
        if (memcmp(I1, I2, N*sizeof(int)) != 0) return 0;
        break;
      }
    case VFF_TYP_FLOAT:
      {
        float *I1 = FLOATDATA(im1);
        float *I2 = FLOATDATA(im2);
        if (memcmp(I1, I2, N*sizeof(float)) != 0) return 0;
        break;
      }
    case VFF_TYP_DOUBLE:
      {
        double *I1 = DOUBLEDATA(im1);
        double *I2 = DOUBLEDATA(im2);
        if (memcmp(I1, I2, N*sizeof(double)) != 0) return 0;
        break;
      }
  } /* switch(f->datatype) */
  return 1;
} // equalimages()

/* ==================================== */
int convertgen(struct xvimage **f1, struct xvimage **f2)
/* ==================================== */
// Converts the images f1, f2 to the type of the most general one.
// Returns the code of the chosen type.
#undef F_NAME
#define F_NAME "convertgen"
{
  struct xvimage *im1 = *f1;
  struct xvimage *im2 = *f2;
  struct xvimage *im3;
  int type1 = datatype(im1);
  int type2 = datatype(im2);
  int type, typemax = max(type1,type2);

  if (type1 == type2) return type1;
  if (type1 < type2) { im1 = *f2;  im2 = *f1; } // im1 : rien a changer
  // il faut convertir 'im2' a 'typemax'
  type = datatype(im2);
  if (type == VFF_TYP_1_BYTE)
  {
    int i, rs=rowsize(im2), cs=colsize(im2), ds=depth(im2), N=rs*cs*ds;
    uint8_t *F = UCHARDATA(im2);
    im3 = allocimage(NULL, rs, cs, ds, typemax);
    if (im3 == NULL)
    {
      fprintf(stderr,"%s : allocimage failed\n", F_NAME);
      return 0;
    }
    if (typemax == VFF_TYP_4_BYTE)
    {
      unsigned int *L = ULONGDATA(im3);
      for (i = 0; i < N; i++) L[i] = (unsigned int)F[i];
    }
    else if (typemax == VFF_TYP_FLOAT)
    {
      float *FL = FLOATDATA(im3);
      for (i = 0; i < N; i++) FL[i] = (float)F[i];
    }
    else
    {
      fprintf(stderr,"%s : bad data type\n", F_NAME);
      return 0;
    }
  }
  else if (type == VFF_TYP_4_BYTE)
  {
    int i, rs=rowsize(im2), cs=colsize(im2), ds=depth(im2), N=rs*cs*ds;
    unsigned int *L = ULONGDATA(im2);
    im3 = allocimage(NULL, rs, cs, ds, typemax);
    if (im3 == NULL)
    {
      fprintf(stderr,"%s : allocimage failed\n", F_NAME);
      return 0;
    }
    if (typemax == VFF_TYP_FLOAT)
    {
      float *FL = FLOATDATA(im3);
      for (i = 0; i < N; i++) FL[i] = (float)L[i];
    }
    else
    {
      fprintf(stderr,"%s : bad data type\n", F_NAME);
      return 0;
    }
  }
  else
  {
    fprintf(stderr,"%s : bad data type\n", F_NAME);
    return 0;
  }

  if (type1 < type2) *f1 = im3; else *f2 = im3;
  freeimage(im2); 
  return typemax; 
} // convertgen()

/* ==================================== */
int convertlong(struct xvimage **f1) 
/* ==================================== */
// Converts the image f1 to int.
#undef F_NAME
#define F_NAME "convertlong"
{
  struct xvimage *im1 = *f1;
  struct xvimage *im3;
  int type = datatype(im1);
  int i, rs=rowsize(im1), cs=colsize(im1), ds=depth(im1), N=rs*cs*ds;
  unsigned int *FL;

  if (type == VFF_TYP_4_BYTE) return 1;

  im3 = allocimage(NULL, rs, cs, ds, VFF_TYP_4_BYTE);
  if (im3 == NULL)
  {
    fprintf(stderr,"%s : allocimage failed\n", F_NAME);
    return 0;
  }
  FL = ULONGDATA(im3);

  if (type == VFF_TYP_1_BYTE)
  {
    uint8_t *F = UCHARDATA(im1);
    for (i = 0; i < N; i++) FL[i] = (unsigned int)F[i];
  }
  else if (type == VFF_TYP_FLOAT)
  {
    float *F = FLOATDATA(im1);
    for (i = 0; i < N; i++) FL[i] = (unsigned int)F[i];
  }
  else
  {
    fprintf(stderr,"%s : bad data type\n", F_NAME);
    return 0;
  }

  *f1 = im3;
  return 1; 
} // convertlong()

/* ==================================== */
int convertfloat(struct xvimage **f1)
/* ==================================== */
// Converts the image f1 to float.
#undef F_NAME
#define F_NAME "convertfloat"
{
  struct xvimage *im1 = *f1;
  struct xvimage *im3;
  int type = datatype(im1);
  int i, rs=rowsize(im1), cs=colsize(im1), ds=depth(im1), N=rs*cs*ds;
  float *FL;

  if (type == VFF_TYP_FLOAT) return 1;

  im3 = allocimage(NULL, rs, cs, ds, VFF_TYP_FLOAT);
  if (im3 == NULL)
  {
    fprintf(stderr,"%s : allocimage failed\n", F_NAME);
    return 0;
  }
  FL = FLOATDATA(im3);

  if (type == VFF_TYP_1_BYTE)
  {
    uint8_t *F = UCHARDATA(im1);
    for (i = 0; i < N; i++) FL[i] = (float)F[i];
  }
  else if (type == VFF_TYP_4_BYTE)
  {
    unsigned int *F = ULONGDATA(im1);
    for (i = 0; i < N; i++) FL[i] = (float)F[i];
  }
  else
  {
    fprintf(stderr,"%s : bad data type\n", F_NAME);
    return 0;
  }

  *f1 = im3;
  return 1; 
} // convertfloat()

/* ==================================== */
void list2image(struct xvimage * image, double *P, int n)
/* ==================================== */
#undef F_NAME
#define F_NAME "list2image"
{
  int rs, cs, ds, ps, N, x, y, z, i;
  uint8_t *F;

  rs = rowsize(image);
  cs = colsize(image);
  ds = depth(image);
  ps = rs * cs;
  N = ps * ds;
  F = UCHARDATA(image);
  if (ds == 1) // 2D
  {
    for (i = 0; i < n; i++)
    {
      x = arrondi(P[2*i]); y = arrondi(P[2*i + 1]); 
      if ((x >= 0) && (x < rs) && (y >= 0) && (y < cs)) 
	F[y * rs + x] = NDG_MAX;
    }
  }
  else // 3D
  {
    for (i = 0; i < n; i++)
    {
      x = arrondi(P[3*i]); 
      y = arrondi(P[3*i + 1]); 
      z = arrondi(P[3*i + 2]); 
      if ((x >= 0) && (x < rs) && (y >= 0) && (y < cs) && (z >= 0) && (z < ds)) 
	F[z*ps + y*rs + x] = NDG_MAX;
    }
  }
} // list2image()

/* ==================================== */
double * image2list(struct xvimage * image, int *n)
/* ==================================== */
#undef F_NAME
#define F_NAME "image2list"
{
  int rs, cs, ds, ps, N, x, y, z;
  uint8_t *F;
  int n1;
  double * P1;

  rs = rowsize(image);
  cs = colsize(image);
  ds = depth(image);
  ps = rs * cs;
  N = ps * ds;
  F = UCHARDATA(image);
  n1 = 0;                     /* compte le nombre de points non nuls pour image1 */ 
  for (x = 0; x < N; x++) if (F[x]) n1++;
  if (ds == 1) // 2D
  {
    P1 = (double *)calloc(1, n1 * 2 * sizeof(double));
    if (P1 == NULL) 
    {   
      fprintf(stderr,"%s : malloc failed for P1\n", F_NAME);
      return NULL;
    }
    n1 = 0;
    for (y = 0; y < cs; y++)
      for (x = 0; x < rs; x++)
	if (F[y * rs + x]) { P1[2*n1] = (double)x; P1[2*n1 + 1] = (double)y; n1++; }
  }
  else // 3D
  {
    P1 = (double *)calloc(1, n1 * 3 * sizeof(double));
    if (P1 == NULL) 
    {   
      fprintf(stderr,"%s : malloc failed for P1\n", F_NAME);
      return NULL;
    }
    n1 = 0;
    for (z = 0; z < ds; z++)
      for (y = 0; y < cs; y++)
	for (x = 0; x < rs; x++)
	  if (F[z*ps + y*rs + x]) 
	  { 
	    P1[3*n1] = (double)x; 
	    P1[3*n1 + 1] = (double)y; 
	    P1[3*n1 + 2] = (double)z; 
	    n1++; 
	  }
  }
  *n = n1;
  return P1;
} // image2list()

/* ==================================== */
void writeimage(struct xvimage * image, char *filename)
/* ==================================== */
#undef F_NAME
#define F_NAME "writeimage"
{
  int rs, cs, ds;
  rs = rowsize(image);
  cs = colsize(image);
  ds = depth(image);
  if ((rs<=25) && (cs<=25) && (ds<=25) &&
      ((datatype(image) == VFF_TYP_1_BYTE) || (datatype(image) == VFF_TYP_4_BYTE) || 
       (datatype(image) == VFF_TYP_FLOAT)))
    writeascimage(image, filename); 
  else
    writerawimage(image, filename); 
} /* writeimage() */

/* ==================================== */
void writerawimage(struct xvimage * image, char *filename)
/* ==================================== */
#undef F_NAME
#define F_NAME "writerawimage"
{
  FILE *fd = NULL;
  int rs, cs, d, N, i, ret;

  rs = rowsize(image);
  cs = colsize(image);
  d = depth(image);
  N = rs * cs * d;

#ifdef UNIXIO
  fd = fopen(filename,"w");
#endif
#ifdef DOSIO
  fd = fopen(filename,"wb");
#endif
  if (!fd)
  {
    fprintf(stderr, "%s: cannot open file: %s\n", F_NAME, filename);
    exit(0);
  }

  if (datatype(image) == VFF_TYP_1_BYTE)
  {
    fputs("P5\n", fd);
    if ((image->xdim != 0.0) && (d > 1))
      fprintf(fd, "#xdim %g\n#ydim %g\n#zdim %g\n", image->xdim, image->ydim, image->zdim);
    if ((image->xdim != 0.0) && (d == 1))
      fprintf(fd, "#xdim %g\n#ydim %g\n", image->xdim, image->ydim);
    if (d > 1) fprintf(fd, "%d %d %d\n", rs, cs, d); else  fprintf(fd, "%d %d\n", rs, cs);
    fprintf(fd, "255\n");

    ret = fwrite(UCHARDATA(image), sizeof(char), N, fd);
    if (ret != N)
    {
      fprintf(stderr, "%s: only %d items written\n", F_NAME, ret);
      exit(0);
    }
  }
  else if (datatype(image) == VFF_TYP_2_BYTE)
  {
    fputs("P5\n", fd);
    if ((image->xdim != 0.0) && (d > 1))
      fprintf(fd, "#xdim %g\n#ydim %g\n#zdim %g\n", image->xdim, image->ydim, image->zdim);
    if ((image->xdim != 0.0) && (d == 1))
      fprintf(fd, "#xdim %g\n#ydim %g\n", image->xdim, image->ydim);
    if (d > 1) fprintf(fd, "%d %d %d\n", rs, cs, d); else  fprintf(fd, "%d %d\n", rs, cs);
    fprintf(fd, "65535\n");

    ret = fwrite(USHORTDATA(image), 2*sizeof(char), N, fd);
    if (ret != N)
    {
      fprintf(stderr, "%s: only %d items written\n", F_NAME, ret);
      exit(0);
    }
  }
  else if (datatype(image) == VFF_TYP_4_BYTE)
  {
    fputs("P8\n", fd);
    if ((image->xdim != 0.0) && (d > 1))
      fprintf(fd, "#xdim %g\n#ydim %g\n#zdim %g\n", image->xdim, image->ydim, image->zdim);
    if ((image->xdim != 0.0) && (d == 1))
      fprintf(fd, "#xdim %g\n#ydim %g\n", image->xdim, image->ydim);
    if (d > 1) fprintf(fd, "%d %d %d\n", rs, cs, d); else  fprintf(fd, "%d %d\n", rs, cs);
    fprintf(fd, "4294967295\n");

    ret = fwrite(ULONGDATA(image), sizeof(int), N, fd);
    if (ret != N)
    {
      fprintf(stderr, "%s: only %d items written\n", F_NAME, ret);
      exit(0);
    }
  }
  else if (datatype(image) == VFF_TYP_FLOAT)
  {
    fputs("P9\n", fd);
    if ((image->xdim != 0.0) && (d > 1))
      fprintf(fd, "#xdim %g\n#ydim %g\n#zdim %g\n", image->xdim, image->ydim, image->zdim);
    if ((image->xdim != 0.0) && (d == 1))
      fprintf(fd, "#xdim %g\n#ydim %g\n", image->xdim, image->ydim);
    if (d > 1) fprintf(fd, "%d %d %d\n", rs, cs, d); else  fprintf(fd, "%d %d\n", rs, cs);
    fprintf(fd, "0\n");

    ret = fwrite(FLOATDATA(image), sizeof(float), N, fd);
    if (ret != N)
    {
      fprintf(stderr, "%s: only %d items written\n", F_NAME, ret);
      exit(0);
    }
  }
  else
  {   fprintf(stderr,"%s : bad datatype : %d\n", F_NAME, datatype(image));
      exit(0);
  }

  fclose(fd);
} /* writerawimage() */

/* ==================================== */
void writese(struct xvimage * image, char *filename, int x, int y, int z)
/* ==================================== */
#undef F_NAME
#define F_NAME "writese"
{
  FILE *fd = NULL;
  int rs, cs, d, ps, N, i, ret;

  rs = rowsize(image);
  cs = colsize(image);
  d = depth(image);
  ps = rs * cs;
  N = ps * d;

#ifdef UNIXIO
  fd = fopen(filename,"w");
#endif
#ifdef DOSIO
  fd = fopen(filename,"wb");
#endif
  if (!fd)
  {
    fprintf(stderr, "%s: cannot open file: %s\n", F_NAME, filename);
    exit(0);
  }

  if (datatype(image) == VFF_TYP_1_BYTE)
  {
    if (d > 1) 
    {
      if ((rs<=25) && (cs<=25) && (d<=25)) fputs("P2\n", fd); else fputs("P5\n", fd); 
      fprintf(fd, "#origin %d %d %d\n", x, y, z);
    }
    else 
    {
      if ((rs<=25) && (cs<=25) && (d<=25)) fputs("P2\n", fd); else fputs("P5\n", fd); 
      fprintf(fd, "#origin %d %d\n", x, y);
    }
    /*    fputs("# CREATOR: writese by MC - 07/1996\n", fd); */
    if (d > 1) fprintf(fd, "%d %d %d\n", rs, cs, d); else  fprintf(fd, "%d %d\n", rs, cs);
    fprintf(fd, "255\n");

    if ((rs<=25) && (cs<=25) && (d<=25))
    {
      for (i = 0; i < N; i++)
      {
	if (i % rs == 0) fprintf(fd, "\n");
	if (i % ps == 0) fprintf(fd, "\n");
	fprintf(fd, "%3d ", (int)(UCHARDATA(image)[i]));
      } /* for i */
      fprintf(fd, "\n");
    }
    else
    {
      ret = fwrite(UCHARDATA(image), sizeof(char), N, fd);
      if (ret != N)
      {
	fprintf(stderr, "%s: only %d items written\n", F_NAME, ret);
	exit(0);
      }
    }
  }
  else
  {   fprintf(stderr,"%s : bad datatype : %d\n", F_NAME, datatype(image));
      exit(0);
  }

  fclose(fd);
} /* writese() */

/* ==================================== */
void writeascimage(struct xvimage * image, char *filename)
/* ==================================== */
#undef F_NAME
#define F_NAME "writeascimage"
{
  FILE *fd = NULL;
  int rs, cs, ps, d, nndg, N, i;

  fd = fopen(filename,"w");
  if (!fd)
  {
    fprintf(stderr, "%s: cannot open file: %s\n", F_NAME, filename);
    exit(0);
  }

  rs = rowsize(image);
  cs = colsize(image);
  d = depth(image);
  ps = rs * cs;
  N = ps * d;

  if (datatype(image) == VFF_TYP_1_BYTE)
  {
    fputs("P2\n", fd);
    if ((image->xdim != 0.0) && (d > 1))
      fprintf(fd, "#xdim %g\n#ydim %g\n#zdim %g\n", image->xdim, image->ydim, image->zdim);
    if (d > 1) fprintf(fd, "%d %d %d\n", rs, cs, d); else  fprintf(fd, "%d %d\n", rs, cs);
    fprintf(fd, "255\n");

    for (i = 0; i < N; i++)
    {
      if (i % rs == 0) fprintf(fd, "\n");
      if (i % ps == 0) fprintf(fd, "\n");
      fprintf(fd, "%3d ", (int)(UCHARDATA(image)[i]));
    } /* for i */
    fprintf(fd, "\n");
  }
  else if (datatype(image) == VFF_TYP_4_BYTE)
  {
    fputs("PB\n", fd);
    if ((image->xdim != 0.0) && (d > 1))
      fprintf(fd, "#xdim %g\n#ydim %g\n#zdim %g\n", image->xdim, image->ydim, image->zdim);
    if (d > 1) fprintf(fd, "%d %d %d\n", rs, cs, d); else  fprintf(fd, "%d %d\n", rs, cs);
    fprintf(fd, "4294967295\n");

    for (i = 0; i < N; i++)
    {
      if (i % rs == 0) fprintf(fd, "\n");
      if (i % ps == 0) fprintf(fd, "\n");
      fprintf(fd, "%ld ", ULONGDATA(image)[i]);
    } /* for i */
    fprintf(fd, "\n");
  }
  else if (datatype(image) == VFF_TYP_FLOAT)
  {
    fputs("PA\n", fd);
    if ((image->xdim != 0.0) && (d > 1))
      fprintf(fd, "#xdim %g\n#ydim %g\n#zdim %g\n", image->xdim, image->ydim, image->zdim);
    if (d > 1) fprintf(fd, "%d %d %d\n", rs, cs, d); else  fprintf(fd, "%d %d\n", rs, cs);
    fprintf(fd, "1\n");

    for (i = 0; i < N; i++)
    {
      if (i % rs == 0) fprintf(fd, "\n");
      if (i % ps == 0) fprintf(fd, "\n");
      fprintf(fd, "%8g ", FLOATDATA(image)[i]);
    } /* for i */
    fprintf(fd, "\n");
  }
  fclose(fd);
}

/* ==================================== */
void printimage(struct xvimage * image)
/* ==================================== */
#undef F_NAME
#define F_NAME "printimage"
{
  int rs, cs, d, ps, N, i;

  rs = rowsize(image);
  cs = colsize(image);
  d = depth(image);
  ps = rs * cs;
  N = ps * d;

  for (i = 0; i < N; i++)
  {
    if (i % rs == 0) printf("\n");
    if (i % ps == 0) printf("\n");
    printf("%3d ", (int)(UCHARDATA(image)[i]));
  } /* for i */
  printf("\n");
}

/* ==================================== */
void writergbimage(
  struct xvimage * redimage,
  struct xvimage * greenimage,
  struct xvimage * blueimage,
  char *filename)
/* ==================================== */
#undef F_NAME
#define F_NAME "writergbimage"
{
  FILE *fd = NULL;
  int rs, cs, nndg, N, i;

#ifdef UNIXIO
  fd = fopen(filename,"w");
#endif
#ifdef DOSIO
  fd = fopen(filename,"wb");
#endif
  if (!fd)
  {
    fprintf(stderr, "%s: cannot open file: %s\n", F_NAME, filename);
    exit(0);
  }

  rs = redimage->row_size;
  cs = redimage->col_size;
  if ((greenimage->row_size != rs) || (greenimage->col_size != cs) ||
      (blueimage->row_size != rs) || (blueimage->col_size != cs))
  {
    fprintf(stderr, "%s: incompatible image sizes\n", F_NAME);
    exit(0);
  }
  
  N = rs * cs;
  nndg = 255;

  fputs("P6\n", fd);
  /*  fputs("# CREATOR: writeimage by MC - 07/1996\n", fd); */
  fprintf(fd, "##rgb\n");
  fprintf(fd, "%d %d\n", rs, cs);
  fprintf(fd, "%d\n", nndg);

  for (i = 0; i < N; i++)
  {
    fputc((int)(UCHARDATA(redimage)[i]), fd);
    fputc((int)(UCHARDATA(greenimage)[i]), fd);
    fputc((int)(UCHARDATA(blueimage)[i]), fd);
  } /* for i */

  fclose(fd);
}

/* ==================================== */
void writelongimage(struct xvimage * image,  char *filename)
/* ==================================== */
#undef F_NAME
#define F_NAME "writelongimage"
/* 
   obsolete - utiliser maintenant writeimage
*/
{
  FILE *fd = NULL;
  int rs, cs, d, nndg, N, i;
  int ret;

#ifdef UNIXIO
  fd = fopen(filename,"w");
#endif
#ifdef DOSIO
  fd = fopen(filename,"wb");
#endif
  if (!fd)
  {
    fprintf(stderr, "%s: cannot open file: %s\n", F_NAME, filename);
    exit(0);
  }

  rs = rowsize(image);
  cs = colsize(image);
  d = depth(image);
  N = rs * cs * d;
  nndg = 255;

  fputs("P8\n", fd);
  /*  fputs("# CREATOR: writelongimage by MC - 07/1996\n", fd); */
  if (d > 1) fprintf(fd, "%d %d %d\n", rs, cs, d); else  fprintf(fd, "%d %d\n", rs, cs);
  fprintf(fd, "%d\n", nndg);

  ret = fwrite(ULONGDATA(image), sizeof(int), N, fd);
  if (ret != N)
  {
    fprintf(stderr, "%s: only %d items written\n", F_NAME, ret);
    exit(0);
  }

  fclose(fd);
} /* writelongimage() */

/* ==================================== */
struct xvimage * readimage(char *filename)
/* ==================================== */
#undef F_NAME
#define F_NAME "readimage"
{
  char buffer[BUFFERSIZE];
  FILE *fd = NULL;
  int rs, cs, d, ndgmax, N, i;
  struct xvimage * image;
  int ascii;  
  int typepixel;
  int c;
  double xdim=1.0, ydim=1.0, zdim=1.0;
  char *read;

#ifdef UNIXIO
  fd = fopen(filename,"r");
#endif
#ifdef DOSIO
  fd = fopen(filename,"rb");
#endif
  if (!fd)
  {
    fprintf(stderr, "%s: file not found: %s\n", F_NAME, filename);
    return NULL;
  }

  read = fgets(buffer, BUFFERSIZE, fd); /* P5: raw byte bw  ; P2: ascii bw */
                                 /* P6: raw byte rgb ; P3: ascii rgb */
                                 /* P8: raw int 2d-3d  ==  extension MC */
                                 /* P9: raw float 2d-3d  ==  extension MC */
                                 /* PA: ascii float 2d-3d  ==  extension LN */
                                 /* PB: ascii int 2d-3d  ==  extension MC */

                                 /* P7: raw byte 3d : OBSOLETE - left for compatibility */
  if (!read)
  {
    fprintf(stderr, "%s: fgets returned without reading\n", F_NAME);
    return 0;
  }

  if (buffer[0] != 'P')
  {   fprintf(stderr,"%s : invalid image format\n", F_NAME);
      return NULL;
  }
  switch (buffer[1])
  {
    case '2': ascii = 1; typepixel = VFF_TYP_1_BYTE; break;
    case '5':
    case '7': ascii = 0; typepixel = VFF_TYP_1_BYTE; break;
    case '8': ascii = 0; typepixel = VFF_TYP_4_BYTE; break;
    case '9': ascii = 0; typepixel = VFF_TYP_FLOAT; break;
    case 'A': ascii = 1; typepixel = VFF_TYP_FLOAT; break;
    case 'B': ascii = 1; typepixel = VFF_TYP_4_BYTE; break;
    default:
      fprintf(stderr,"%s : invalid image format : P%c\n", F_NAME, buffer[1]);
      return NULL;
  } /* switch */

  do
  {
    read = fgets(buffer, BUFFERSIZE, fd); /* commentaire */
    if (!read)
    {
      fprintf(stderr, "%s: fgets returned without reading\n", F_NAME);
      return 0;
    }
    if (strncmp(buffer, "#xdim", 5) == 0)
      sscanf(buffer+5, "%lf", &xdim);
    else if (strncmp(buffer, "#ydim", 5) == 0)
      sscanf(buffer+5, "%lf", &ydim);
    else if (strncmp(buffer, "#zdim", 5) == 0)
      sscanf(buffer+5, "%lf", &zdim);
  } while (!isdigit(buffer[0]));

  c = sscanf(buffer, "%d %d %d", &rs, &cs, &d);
  if (c == 2) d = 1;
  else if (c != 3)
  {   fprintf(stderr,"%s : invalid image format\n", F_NAME);
      return NULL;
  }

  read = fgets(buffer, BUFFERSIZE, fd);
  if (!read)
  {
    fprintf(stderr, "%s: fgets returned without reading\n", F_NAME);
    return 0;
  }

  sscanf(buffer, "%d", &ndgmax);
  N = rs * cs * d;

  image = allocimage(NULL, rs, cs, d, typepixel);
  if (image == NULL)
  {   fprintf(stderr,"%s : alloc failed\n", F_NAME);
      return(NULL);
  }
  image->xdim = xdim;
  image->ydim = ydim;
  image->zdim = zdim;

  if (typepixel == VFF_TYP_1_BYTE)
  {
    if (ascii)
    {
      if (ndgmax == 255)
        for (i = 0; i < N; i++)
        {
          fscanf(fd, "%d", &c);
          UCHARDATA(image)[i] = (uint8_t)c;
        } /* for i */
      else if (ndgmax == 65535)
        for (i = 0; i < N; i++)
        {
          fscanf(fd, "%d", &c);
          UCHARDATA(image)[i] = (uint8_t)(c/256);
        } /* for i */
      else
      {
        fprintf(stderr,"%s : wrong ndgmax = %d\n", F_NAME, ndgmax);
        return(NULL);
      }
    }
    else
    {
      int ret = fread(UCHARDATA(image), sizeof(char), N, fd);
      if (ret != N)
      {
        fprintf(stderr,"%s : fread failed : %d asked ; %d read\n", F_NAME, N, ret);
        return(NULL);
      }
    }
  } /* if (typepixel == VFF_TYP_1_BYTE) */
  else
  if (typepixel == VFF_TYP_4_BYTE)
  {
    if (ascii)
    {
      for (i = 0; i < N; i++)
      {
        fscanf(fd, "%ld", &(ULONGDATA(image)[i]));
      } /* for i */
    }
    else 
    {
      int ret = fread(ULONGDATA(image), sizeof(int), N, fd);
      if (ret != N)
      {
        fprintf(stderr,"%s : fread failed : %d asked ; %d read\n", F_NAME, N, ret);
        return(NULL);
      }
    }
  } /* if (typepixel == VFF_TYP_4_BYTE) */
  else
  if (typepixel == VFF_TYP_FLOAT)
  {
    if (ascii)
    {
      for (i = 0; i < N; i++)
      {
        fscanf(fd, "%f", &(FLOATDATA(image)[i]));
      } /* for i */
    }
    else 
    {
      int ret = fread(FLOATDATA(image), sizeof(float), N, fd);
      if (ret != N)
      {
        fprintf(stderr,"%s : fread failed : %d asked ; %d read\n", F_NAME, N, ret);
        return(NULL);
      }
    }
  } /* if (typepixel == VFF_TYP_FLOAT) */

  fclose(fd);

  return image;
} /* readimage() */

/* ==================================== */
struct xvimage * readheader(char *filename)
/* ==================================== */
#undef F_NAME
#define F_NAME "readheader"
{
  char buffer[BUFFERSIZE];
  FILE *fd = NULL;
  int rs, cs, d, ndgmax, N, i;
  struct xvimage * image;
  int ascii;  
  int typepixel;
  int c;
  double xdim=1.0, ydim=1.0, zdim=1.0;
  char *read;

#ifdef UNIXIO
  fd = fopen(filename,"r");
#endif
#ifdef DOSIO
  fd = fopen(filename,"rb");
#endif
  if (!fd)
  {
    fprintf(stderr, "%s: file not found: %s\n", F_NAME, filename);
    return NULL;
  }

  read = fgets(buffer, BUFFERSIZE, fd);
  if (!read)
  {
    fprintf(stderr, "%s: fgets returned without reading\n", F_NAME);
    return 0;
  }
  if (buffer[0] != 'P')
  {
    fprintf(stderr,"%s : invalid image format: %c%c\n", F_NAME, buffer[0], buffer[1]);
    return NULL;
  }
  switch (buffer[1])
  {
    case '2': ascii = 1; typepixel = VFF_TYP_1_BYTE; break;
    case '5':
    case '7': ascii = 0; typepixel = VFF_TYP_1_BYTE; break;
    case '8': ascii = 0; typepixel = VFF_TYP_4_BYTE; break;
    case '9': ascii = 0; typepixel = VFF_TYP_FLOAT; break;
    case 'A': ascii = 1; typepixel = VFF_TYP_FLOAT; break;
    case 'B': ascii = 1; typepixel = VFF_TYP_4_BYTE; break;
    default:
      fprintf(stderr,"%s : invalid image format: %c%c\n", F_NAME, buffer[0], buffer[1]);
      return NULL;
  } /* switch */

  do 
  {
    read = fgets(buffer, BUFFERSIZE, fd); /* commentaire */
    if (!read)
    {
      fprintf(stderr, "%s: fgets returned without reading\n", F_NAME);
      return 0;
    }
    if (strncmp(buffer, "#xdim", 5) == 0)
      sscanf(buffer+5, "%lf", &xdim);
    else if (strncmp(buffer, "#ydim", 5) == 0)
      sscanf(buffer+5, "%lf", &ydim);
    else if (strncmp(buffer, "#zdim", 5) == 0)
      sscanf(buffer+5, "%lf", &zdim);
  } while (!isdigit(buffer[0]));

  c = sscanf(buffer, "%d %d %d", &rs, &cs, &d);
  if (c == 2) d = 1;
  else if (c != 3)
  {   fprintf(stderr,"%s : invalid image format\n", F_NAME);
      return NULL;
  }

  read = fgets(buffer, BUFFERSIZE, fd);
  if (!read)
  {
    fprintf(stderr, "%s: fgets returned without reading\n", F_NAME);
    return 0;
  }

  sscanf(buffer, "%d", &ndgmax);

  image = allocheader(NULL, rs, cs, d, typepixel);
  if (image == NULL)
  {   fprintf(stderr,"%s : alloc failed\n", F_NAME);
      return(NULL);
  }
  image->xdim = xdim;
  image->ydim = ydim;
  image->zdim = zdim;

  fclose(fd);

  return image;
} /* readheader() */

/* ==================================== */
struct xvimage * readse(char *filename, int *x, int *y, int*z)
/* ==================================== */
#undef F_NAME
#define F_NAME "readse"
/*
Specialisation de readimage pour les elements structurants.
L'origine est donnee dans le fichier pgm par une ligne de commentaire
de la forme : 
#origin x y [z]
*/
{
  char buffer[BUFFERSIZE];
  FILE *fd = NULL;
  int rs, cs, d, ndgmax, N, i;
  struct xvimage * image;
  int ascii;  
  int typepixel;
  int c;
  int dimorigin = 0;
  char *read;

#ifdef UNIXIO
  fd = fopen(filename,"r");
#endif
#ifdef DOSIO
  fd = fopen(filename,"rb");
#endif
  if (!fd)
  {
    fprintf(stderr, "%s: file not found: %s\n", F_NAME, filename);
    return NULL;
  }

  read = fgets(buffer, BUFFERSIZE, fd);
  if (!read)
  {
    fprintf(stderr, "%s: fgets returned without reading\n", F_NAME);
    return 0;
  }
  if (buffer[0] != 'P')
  {   fprintf(stderr,"%s : invalid image format\n", F_NAME);
      return NULL;
  }
  switch (buffer[1])
  {
    case '2': ascii = 1; typepixel = VFF_TYP_1_BYTE; break;
    case '5':
    case '7': ascii = 0; typepixel = VFF_TYP_1_BYTE; break;
    default:
      fprintf(stderr,"%s : invalid image format\n", F_NAME);
      return NULL;
  } /* switch */

  do 
  {
    read = fgets(buffer, BUFFERSIZE, fd); /* commentaire */
    if (!read)
    {
      fprintf(stderr, "%s: fgets returned without reading\n", F_NAME);
      return 0;
    }
    if (strncmp(buffer, "#origin", 7) == 0)
    {
      dimorigin = sscanf(buffer+7, "%d %d %d", x, y, z);
#ifdef VERBOSE
      if (dimorigin == 2) fprintf(stderr, "%s: origin %d %d\n", F_NAME, *x, *y);
      if (dimorigin == 3) fprintf(stderr, "%s: origin %d %d %d\n", F_NAME, *x, *y, *z);
#endif
    }
  } while (buffer[0] == '#');

  if (!dimorigin)
  {   fprintf(stderr,"%s : origin missing for structuring element\n", F_NAME);
      return NULL;
  }

  c = sscanf(buffer, "%d %d %d", &rs, &cs, &d);
  if (c != dimorigin)
  {   fprintf(stderr,"%s : incompatible origin and image dimensions\n", F_NAME);
      return NULL;
  }
  if (c == 2) d = 1;
  else if (c != 3)
  {   fprintf(stderr,"%s : invalid image format\n", F_NAME);
      return NULL;
  }

  read = fgets(buffer, BUFFERSIZE, fd);
  if (!read)
  {
    fprintf(stderr, "%s: fgets returned without reading\n", F_NAME);
    return 0;
  }

  sscanf(buffer, "%d", &ndgmax);
  N = rs * cs * d;

  image = allocimage(NULL, rs, cs, d, typepixel);
  if (image == NULL)
  {   fprintf(stderr,"%s : alloc failed\n", F_NAME);
      return(NULL);
  }

  if (typepixel == VFF_TYP_1_BYTE)
  {
    if (ascii)
    {
      if (ndgmax == 255)
        for (i = 0; i < N; i++)
        {
          fscanf(fd, "%d", &c);
          UCHARDATA(image)[i] = (uint8_t)c;
        } /* for i */
      else if (ndgmax == 65535)
        for (i = 0; i < N; i++)
        {
          fscanf(fd, "%d", &c);
          UCHARDATA(image)[i] = (uint8_t)(c/256);
        } /* for i */
      else
      {   fprintf(stderr,"%s : wrong ndgmax = %d\n", F_NAME, ndgmax);
          return(NULL);
      }
    }
    else
    {
      int ret = fread(UCHARDATA(image), sizeof(char), N, fd);
      if (ret != N)
      {
        fprintf(stderr,"%s : fread failed : %d asked ; %d read\n", F_NAME, N, ret);
        return(NULL);
      }
    }
  } /* if (typepixel == VFF_TYP_1_BYTE) */
  else
  if (typepixel == VFF_TYP_4_BYTE)
  {
    int ret = fread(ULONGDATA(image), sizeof(int), N, fd);
    if (ret != N)
    {
      fprintf(stderr,"%s : fread failed : %d asked ; %d read\n", F_NAME, N, ret);
      return(NULL);
    }
  } /* if (typepixel == VFF_TYP_4_BYTE) */

  fclose(fd);

  return image;
} /* readse() */

void readSeList(char *esname, int **x, int **y, int *n){
  struct xvimage * es;
  unsigned char *M;
  int rsm;        /* taille ligne masque */
  int csm;        /* taille colonne masque */
  int Nm;         /* nbre de points dans le masque */
  int k;          /* Index muet */
  int nbpt;       /* nbre de point dans l'e.s. */
  
  int c_i, c_j, c_z;            /* coordonn�e de centre de l'�l�ment structurant */
  
  int i,j;                      /* coordonn�es d'un point */

  int *tab_es_i;               /* liste des coord. x des points de l'e.s. */
  int *tab_es_j;               /* liste des coord. y des points de l'e.s. */

  es = readse(esname, &c_i, &c_j, &c_z);
  if ( (es == NULL))
  {
    fprintf(stderr, "%s: readse failed\n");
    exit(1);
  }
  
  M = UCHARDATA(es);
  rsm = rowsize(es);        /* taille ligne masque */
  csm = colsize(es);        /* taille colonne masque */
  Nm = rsm * csm;

  nbpt = 0;
  for (k = 0; k < Nm; k ++)
    if (M[k])
      nbpt += 1;

  tab_es_i = (int *)calloc(1,nbpt * sizeof(int));
  tab_es_j = (int *)calloc(1,nbpt * sizeof(int));
  if ((tab_es_i == NULL) || (tab_es_j == NULL))
  {  
     fprintf(stderr,"malloc failed for tab_es\n");
     exit(0);
  }

  k = 0;
  
  for (j = 0; j < csm; j += 1)
    for (i = 0; i < rsm; i += 1)
      if (M[j * rsm + i] == VRAI)
      {
	tab_es_i[k] = i - c_i;  
	tab_es_j[k] = j - c_j;
	k++;
      }
  
  (*x) = tab_es_i;
  (*y) = tab_es_j;
  (*n) = k;
  freeimage(es);
}

/* ==================================== */
int readrgbimage(
  char *filename,
  struct xvimage ** r,
  struct xvimage ** g,
  struct xvimage ** b)
/* ==================================== */
#undef F_NAME
#define F_NAME "readrgbimage"
{
  char buffer[BUFFERSIZE];
  FILE *fd = NULL;
  int rs, cs, nndg, N, i;
  int ascii = 0;  
  int c;
  char *read;

#ifdef UNIXIO
  fd = fopen(filename,"r");
#endif
#ifdef DOSIO
  fd = fopen(filename,"rb");
#endif
  if (!fd)
  {
    fprintf(stderr, "%s: file not found: %s\n", F_NAME, filename);
    return 0;
  }

  read = fgets(buffer, BUFFERSIZE, fd); /* P5: raw int bw  ; P2: ascii bw */
                                        /* P6: raw int rgb ; P3: ascii rgb */
  if (!read)
  {
    fprintf(stderr, "%s: fgets returned without reading\n", F_NAME);
    return 0;
  }
  if (buffer[0] != 'P')
  {   fprintf(stderr,"%s : invalid image format\n", F_NAME);
      return 0;
  }

  switch (buffer[1])
  {
    case '3': ascii = 1; break;
    case '6': ascii = 0; break;
    default:
      fprintf(stderr,"%s : invalid image format\n", F_NAME);
      return 0;
  } /* switch */

  do 
  {
    read = fgets(buffer, BUFFERSIZE, fd); /* commentaire */
    if (!read)
    {
      fprintf(stderr, "%s: fgets returned without reading\n", F_NAME);
      return 0;
    }
  } while (!isdigit(buffer[0]));

  c = sscanf(buffer, "%d %d", &rs, &cs);
  if (c != 2)
  {   fprintf(stderr,"%s : invalid image format\n", F_NAME);
      return 0;
  }

  read = fgets(buffer, BUFFERSIZE, fd);
  if (!read)
  {
    fprintf(stderr, "%s: fgets returned without reading\n", F_NAME);
    return 0;
  }
  sscanf(buffer, "%d", &nndg);
  N = rs * cs;

  *r = allocimage(NULL, rs, cs, 1, VFF_TYP_1_BYTE);
  *g = allocimage(NULL, rs, cs, 1, VFF_TYP_1_BYTE);
  *b = allocimage(NULL, rs, cs, 1, VFF_TYP_1_BYTE);
  if ((*r == NULL) && (*g == NULL) && (*b == NULL))
  {   fprintf(stderr,"%s : allocimage failed\n", F_NAME);
      return(0);
  }

  if (ascii)
    for (i = 0; i < N; i++)
    {
      fscanf(fd, "%d", &c);
      (UCHARDATA(*r))[i] = (uint8_t)c;
      fscanf(fd, "%d", &c);
      (UCHARDATA(*g))[i] = (uint8_t)c;
      fscanf(fd, "%d", &c);
      (UCHARDATA(*b))[i] = (uint8_t)c;
    } /* for i */
  else
    for (i = 0; i < N; i++)
    {
      (UCHARDATA(*r))[i] = fgetc(fd);    
      (UCHARDATA(*g))[i] = fgetc(fd);    
      (UCHARDATA(*b))[i] = fgetc(fd);    
    } /* for i */

  fclose(fd);
  return 1;
} /* readrgbimage() */

/* ==================================== */
struct xvimage * readlongimage(char *filename)
/* ==================================== */
#undef F_NAME
#define F_NAME "readlongimage"
/* 
   obsolete - utiliser maintenant readimage
*/
{
  char buffer[BUFFERSIZE];
  FILE *fd = NULL;
  int rs, cs, d, nndg, N, i;
  struct xvimage * image;
  int c, ret;
  char *read;

#ifdef UNIXIO
  fd = fopen(filename,"r");
#endif
#ifdef DOSIO
  fd = fopen(filename,"rb");
#endif
  if (!fd)
  {
    fprintf(stderr, "%s: file not found: %s\n", F_NAME, filename);
    return NULL;
  }

  read = fgets(buffer, BUFFERSIZE, fd); /* P8: raw int 3d  ==  extension MC */
  if (!read)
  {
    fprintf(stderr, "%s: fgets returned without reading\n", F_NAME);
    return 0;
  }

  if ((buffer[0] != 'P') || (buffer[1] != '8'))
  {   fprintf(stderr,"%s : invalid image format\n", F_NAME);
      return NULL;
  }

  do 
  {
    read = fgets(buffer, BUFFERSIZE, fd); /* commentaire */
    if (!read)
    {
      fprintf(stderr, "%s: fgets returned without reading\n", F_NAME);
      return 0;
    }
  } while (buffer[0] == '#');

  c = sscanf(buffer, "%d %d %d", &rs, &cs, &d);
  if (c == 2) d = 1;
  else if (c != 3)
  {   fprintf(stderr,"%s : invalid image format - c = %d \n", F_NAME, c);
      return NULL;
  }

  read = fgets(buffer, BUFFERSIZE, fd);
  if (!read)
  {
    fprintf(stderr, "%s: fgets returned without reading\n", F_NAME);
    return 0;
  }

  sscanf(buffer, "%d", &nndg);
  N = rs * cs * d;

  image = allocimage(NULL, rs, cs, d, VFF_TYP_4_BYTE);
  if (image == NULL)
  {   fprintf(stderr,"%s : alloc failed\n", F_NAME);
      return(NULL);
  }

  ret = fread(ULONGDATA(image), sizeof(int), N, fd);
  if (ret != N)
  {
    fprintf(stderr,"%s : fread failed : %d asked ; %d read\n", F_NAME, N, ret);
    return(NULL);
  }

  fclose(fd);
  return image;
} /* readlongimage() */

/* =========================================================================== */
/* =========================================================================== */
/* BMP files */
/* =========================================================================== */
/* =========================================================================== */

struct BITMAPFILEHEADER {   /* size 14 bytes */
  char Signature[2];        /* size 2 bytes : 'BM' */
  unsigned int FileSize;   /* size 4 bytes : File size in bytes */
  unsigned int reserved;   /* size 4 bytes : unused (=0) */
  unsigned int DataOffset; /* size 4 bytes : File offset to Raster Data */
};

struct BITMAPINFOHEADER {    /* size 40 bytes */
  unsigned int Size;        /* size 4 bytes : Size of InfoHeader =40 */  
  unsigned int Width;       /* size 4 bytes : Bitmap Width */
  unsigned int Height;      /* size 4 bytes : Bitmap Height */
  uint16_t Planes;     /* size 2 bytes : Number of Planes (=1) */
  uint16_t BitCount;   /* size 2 bytes : Bits per Pixel    */
                             /*   1 = monochrome palette. NumColors = 1   
                                  4 = 4bit palletized. NumColors = 16   
                                  8 = 8bit palletized. NumColors = 256  
                                  16 = 16bit RGB. NumColors = 65536 (?)  
                                  24 = 24bit RGB. NumColors = 16M
			     */
  unsigned int Compression; /* size 4 bytes : Type of Compression    */
                             /*
                                  0 = BI_RGB   no compression   
                                  1 = BI_RLE8 8bit RLE encoding   
                                  2 = BI_RLE4 4bit RLE encoding
			     */
  unsigned int ImageSize;   /* size 4 bytes : (compressed) Size of Image   */
                             /* It is valid to set this =0 if Compression = 0 */
  unsigned int XpixelsPerM; /* size 4 bytes : horizontal resolution: Pixels/meter */
  unsigned int YpixelsPerM; /* size 4 bytes : vertical resolution: Pixels/meter */
  unsigned int ColorsUsed;  /* size 4 bytes : Number of actually used colors */
  unsigned int ColorsImportant; /* size 4 bytes : Number of important colors (0 = all) */
};

/*
       ColorTable
                      4 * NumColors bytes
                                        present only if Info.BitsPerPixel <= 8   
                                        colors should be ordered by importance
        
          Red
                      1 byte
                                        Red intensity
          Green
                      1 byte
                                        Green intensity
          Blue
                      1 byte
                                        Blue intensity
          reserved
                      1 byte
                                        unused (=0)
         repeated NumColors times

       Raster Data
                      Info.ImageSize bytes
                                        The pixel data

Raster Data encoding

       Depending on the image's BitCount and on the Compression flag 
       there are 6 different encoding schemes. All of them share the
       following:  

       Pixels are stored bottom-up, left-to-right. Pixel lines are 
       padded with zeros to end on a 32bit (4byte) boundary. For
       uncompressed formats every line will have the same number of bytes. 
       Color indices are zero based, meaning a pixel color of 0
       represents the first color table entry, a pixel color of 255 
       (if there are that many) represents the 256th entry. For images with more
       than 256 colors there is no color table. 

                                                            
       Raster Data encoding for 1bit / black & white images

       BitCount = 1 Compression = 0  
       Every byte holds 8 pixels, its highest order bit representing 
       the leftmost pixel of those. There are 2 color table entries. Some
       readers will ignore them though, and assume that 0 is black and 1 is white. 
       If you are storing black and white pictures you should
       stick to this, with any other 2 colors this is not an issue. 
       Remember padding with zeros up to a 32bit boundary (This can be up to
       31 zeros/pixels!) 

                                                            
       Raster Data encoding for 4bit / 16 color images

       BitCount = 4 Compression = 0  
       Every byte holds 2 pixels, its high order 4 bits representing the left of those. 
       There are 16 color table entries. These colors do not
       have to be the 16 MS-Windows standard colors. Padding each line with 
       zeros up to a 32bit boundary will result in up to 28 zeros
       = 7 'wasted pixels'.

                                                            
       Raster Data encoding for 8bit / 256 color images

       BitCount = 8 Compression = 0  
       Every byte holds 1 pixel. There are 256 color table entries. 
       Padding each line with zeros up to a 32bit boundary will result in up to
       3 bytes of zeros = 3 'wasted pixels'.

                                                            
       Raster Data encoding for 16bit / hicolor images

       BitCount = 16 Compression = 0  
       Every 2bytes / 16bit holds 1 pixel.   
       <information missing: the 16 bit was introduced together with 
       Video For Windows? Is it a memory-only-format?>  
       The pixels are no color table pointers. There are no color table entries. 
       Padding each line with zeros up to a 16bit boundary will
       result in up to 2 zero bytes.

                                                            
       Raster Data encoding for 24bit / truecolor images

       BitCount = 24 Compression = 0  
       Every 4bytes / 32bit holds 1 pixel. The first holds its red, 
       the second its green, and the third its blue intensity. The fourth byte is
       reserved and should be zero. There are no color table entries. 
       The pixels are no color table pointers. No zero padding necessary.

                                                            
       Raster Data compression for 4bit / 16 color images

       BitCount = 4 Compression = 2  
       The pixel data is stored in 2bytes / 16bit chunks.  The first of these 
       specifies the number of consecutive pixels with the same pair
       of color. The second byte defines two color indices. The resulting 
       pixel pattern will be interleaved high-order 4bits and low order 4
       bits (ABABA...). If the first byte is zero, the second defines an escape code. 
       The End-of-Bitmap is zero padded to end on a 32bit boundary. 
       Due to the 16bit-ness of this structure this will always be either 
       two zero bytes or none.   
         
       n (byte 1)
                c (Byte 2)
                                                               Description
       >0
                any
                         n pixels are to be drawn. The 1st, 3rd, 5th, ... pixels' 
                         color is in c's high-order 4 bits, the even pixels' color
                         is in c's low-order 4 bits. If both color indices are the same, 
                         it results in just n pixels of color c
       0
                0
                         End-of-line
       0
                1
                         End-of-Bitmap
       0
                2
                         Delta. The following 2 bytes define an unsigned offset 
                         in x and y direction (y being up) The skipped pixels
                         should get a color zero.
       0
                >=3
                         The following c bytes will be read as single pixel colors 
                         just as in uncompressed files. up to 12 bits of zeros
                         follow, to put the file/memory pointer on a 16bit boundary again.

         
                                 Example for 4bit RLE
       Compressed Data
                                           Expanded data
       03 04
                        0 4 0
       05 06
                        0 6 0 6 0
       00 06 45 56 67 00
                        4 5 5 6 6 7
       04 78
                        7 8 7 8
       00 02 05 01
                        Move 5 right and 1 up. (Windows docs say down, which is wrong)
       00 00
                        End-of-line
       09 1E
                        1 E 1 E 1 E 1 E 1
       00 01
                        EndofBitmap
       00 00
                        Zero padding for 32bit boundary

        

                                                            
       Raster Data compression for 8bit / 256 color images

       BitCount = 8 Compression = 1  
       The pixel data is stored in 2bytes / 16bit chunks.  The first of these 
       specifies the number of consecutive pixels with the same
       color. The second byte defines their color index. If the first byte 
       is zero, the second defines an escape code. The End-of-Bitmap
       is zero padded to end on a 32bit boundary. Due to the 16bit-ness of 
       this structure this will always be either two zero bytes or none.   
         
       n (byte 1)
                c (Byte 2)
                                                               Description
       >0
                any
                         n pixels of color number c
       0
                0
                         End-of-line
       0
                1
                         EndOfBitmap
       0
                2
                         Delta. The following 2 bytes define an unsigned offset 
                         in x and y direction (y being up) The skipped pixels
                         should get a color zero.
       0
                >=3
                         The following c bytes will be read as single pixel colors 
                         just as in uncompressed files. A zero follows, if c is
                         odd, putting the file/memory pointer on a 16bit boundary again.

         
                                 Example for 8bit RLE
       Compressed Data
                                           Expanded data
       03 04
                        04 04 04
       05 06
                        06 06 06 06 06
       00 03 45 56 67 00
                        45 56 67
       02 78
                        78 78
       00 02 05 01
                        Move 5 right and 1 up. (Windows docs say down, which is wrong)
       00 00
                        End-of-line
       09 1E
                        1E 1E 1E 1E 1E 1E 1E 1E 1E
       00 01
                        End-of-bitmap
       00 00
                        Zero padding for 32bit boundary

*/

void freadushort(uint16_t *ptr, FILE* fd)
{
  uint16_t tmp; uint8_t t1, t2;
  t1 = getc(fd);  t2 = getc(fd);
  tmp = t2;
  tmp = tmp*256+t1;
  *ptr = tmp;
}

void freadulong(unsigned int *ptr, FILE* fd)
{
  unsigned int tmp; uint8_t t1, t2, t3, t4;
  t1 = getc(fd);  t2 = getc(fd);  t3 = getc(fd);  t4 = getc(fd);
  tmp = t4;
  tmp = tmp*256+t3;
  tmp = tmp*256+t2;
  tmp = tmp*256+t1;
  *ptr = tmp;
}

/* =============================================================== */
int readbmp(char *filename, struct xvimage ** r, struct xvimage ** g, struct xvimage ** b)
/* =============================================================== */
#undef F_NAME
#define F_NAME "readbmp"
{
  FILE *fd;
  struct BITMAPFILEHEADER FileHeader;
  struct BITMAPINFOHEADER InfoHeader;
  uint8_t *R, *G, *B;
  int i, j, rs, cs;

#ifdef DOSIO
  fd = fopen(filename,"rb");
#endif
#ifdef UNIXIO
  fd = fopen(filename,"r");
#endif
  if (!fd)
  {
    fprintf(stderr, "%s: cannot open file: %s\n", F_NAME, filename);
    return 0;
  }

  fread(&(FileHeader.Signature), sizeof(char), 2, fd);
  freadulong(&(FileHeader.FileSize), fd);
  freadulong(&(FileHeader.reserved), fd);
  freadulong(&(FileHeader.DataOffset), fd);
#ifdef VERBOSE
  printf("signature = %c%c\n", FileHeader.Signature[0], FileHeader.Signature[1]);
  printf("file size = %ld\n", FileHeader.FileSize);
  printf("reserved = %ld\n", FileHeader.reserved);
  printf("data offset = %ld\n", FileHeader.DataOffset);
#endif
  freadulong(&(InfoHeader.Size), fd);
  freadulong(&(InfoHeader.Width), fd);
  freadulong(&(InfoHeader.Height), fd);
  freadushort(&(InfoHeader.Planes), fd);
  freadushort(&(InfoHeader.BitCount), fd);
  freadulong(&(InfoHeader.Compression), fd);
  freadulong(&(InfoHeader.ImageSize), fd);
  freadulong(&(InfoHeader.XpixelsPerM), fd);
  freadulong(&(InfoHeader.YpixelsPerM), fd);
  freadulong(&(InfoHeader.ColorsUsed), fd);
  freadulong(&(InfoHeader.ColorsImportant), fd);
#ifdef VERBOSE
  printf("Size = %d\n", InfoHeader.Size);
  printf("Width = %d\n", InfoHeader.Width);
  printf("Height = %d\n", InfoHeader.Height);
  printf("Planes = %d\n", InfoHeader.Planes);
  printf("BitCount = %d\n", InfoHeader.BitCount);
  printf("Compression = %d\n", InfoHeader.Compression);
  printf("ImageSize = %d\n", InfoHeader.ImageSize);
  printf("XpixelsPerM = %d\n", InfoHeader.XpixelsPerM);
  printf("YpixelsPerM = %d\n", InfoHeader.YpixelsPerM);
  printf("ColorsUsed = %d\n", InfoHeader.ColorsUsed);
  printf("ColorsImportant = %d\n", InfoHeader.ColorsImportant);
#endif
  if ((InfoHeader.Compression != 0) && (InfoHeader.BitCount != 24))
  {
    fprintf(stderr, "restricted bmp format conversion:\n");
    fprintf(stderr, "compression tag must be 0 (No compression), found: %d\n", InfoHeader.Compression);
    fprintf(stderr, "bitcount/pixel must be 24 (True color), found: %d\n", InfoHeader.BitCount);
    fprintf(stderr, "cannot process file\n");
    return 0;
  }
  rs = InfoHeader.Width;
  cs = InfoHeader.Height;
  *r = allocimage(NULL, rs, cs, 1, VFF_TYP_1_BYTE); 
  *g = allocimage(NULL, rs, cs, 1, VFF_TYP_1_BYTE); 
  *b = allocimage(NULL, rs, cs, 1, VFF_TYP_1_BYTE); 
  if ((*r == NULL) || (*g == NULL) || (*b == NULL))
  {
    fprintf(stderr, "%s: allocimage failed\n", F_NAME);
    return 0;
  }
  R = ((UCHARDATA(*r)));
  G = ((UCHARDATA(*g)));
  B = ((UCHARDATA(*b)));
  for (j = cs-1; j >= 0 ; j--)
  for (i = 0; i < rs; i++)
  {
    B[(j*rs)+i] = (uint8_t)getc(fd);
    G[(j*rs)+i] = (uint8_t)getc(fd);
    R[(j*rs)+i] = (uint8_t)getc(fd);
    /* (void)getc(fd); */
  }
  fclose(fd);
  return 1;
} /* readbmp() */

void fwriteushort(uint16_t us, FILE* fd)
{
  putc((uint8_t)(us & 0xff), fd); us = us >> 8;
  putc((uint8_t)(us & 0xff), fd); us = us >> 8;
}

void fwriteulong(unsigned int ul, FILE* fd)
{
  putc((uint8_t)(ul & 0xff), fd); ul = ul >> 8;
  putc((uint8_t)(ul & 0xff), fd); ul = ul >> 8;
  putc((uint8_t)(ul & 0xff), fd); ul = ul >> 8;
  putc((uint8_t)(ul & 0xff), fd); ul = ul >> 8;
}

/* =============================================================== */
void writebmp(
  struct xvimage * redimage,
  struct xvimage * greenimage,
  struct xvimage * blueimage,
  char *filename)
/* =============================================================== */
#undef F_NAME
#define F_NAME "writebmp"
{
  FILE *fd;
  uint8_t *R, *G, *B;
  int i, j, rs, cs, N;

  rs = rowsize(redimage);
  cs = colsize(redimage);
  N = rs * cs;

  if ((rs != rowsize(greenimage)) || (cs != colsize(greenimage)) || 
      (rs != rowsize(blueimage)) || (cs != colsize(blueimage)))
  {
    fprintf(stderr, "%s: incompatible image sizes\n", F_NAME);
    exit(0);
  }

  R = UCHARDATA(redimage);
  G = UCHARDATA(greenimage);
  B = UCHARDATA(blueimage);

#ifdef DOSIO
  fd = fopen(filename,"wb");
#endif
#ifdef UNIXIO
  fd = fopen(filename,"w");
#endif
  if (!fd)
  {
    fprintf(stderr, "%s: cannot open file: %s\n", F_NAME, filename);
    exit(0);
  }

  putc('B', fd); putc('M', fd);
  fwriteulong(N + 54, fd);
  fwriteulong(0, fd);
  fwriteulong(54, fd);
  fwriteulong(40, fd);
  fwriteulong(rs, fd);
  fwriteulong(cs, fd);
  fwriteushort(1, fd);
  fwriteushort(24, fd);
  fwriteulong(0, fd);
  fwriteulong(N, fd);
  fwriteulong(0, fd);
  fwriteulong(0, fd);
  fwriteulong(0, fd);
  fwriteulong(0, fd);

  for (j = cs-1; j >= 0 ; j--)
  for (i = 0; i < rs; i++)
  {
    putc(B[(j*rs)+i], fd);
    putc(G[(j*rs)+i], fd);
    putc(R[(j*rs)+i], fd);
  }
  
  fclose(fd);
} /* writebmp() */

/* =========================================================================== */
/* =========================================================================== */
/* RGB files */
/* =========================================================================== */
/* =========================================================================== */

/* RGB file format is documented on:

http://reality.sgi.com/grafica/sgiimage.html

*/

struct RGBFILEHEADER {       /* size 108 bytes */
  uint16_t magic;      /* size 2 bytes : magic number = 474 */
  uint8_t compression; /* size 1 byte : 0 for no compression */
  uint8_t bytespercha; /* size 1 byte : nb. bytes per channel */
  uint16_t dim;        /* size 2 bytes : nb. channels */
  uint16_t width;      /* size 2 bytes : image row size */
  uint16_t height;     /* size 2 bytes : image col size */
  uint16_t components; /* size 2 bytes : components */
  unsigned int mincol;      /* size 4 bytes : 0 */
  unsigned int maxcol;      /* size 4 bytes : 255 */
  unsigned int dummy;       /* size 4 bytes : dummy */
  char name[80];             /* size 80 bytes : image name or comment */
  unsigned int cmaptype;    /* size 4 bytes : 0 for NORMAL RGB */
}; /** plus 404 bytes dummy padding to make header 512 bytes **/

/* =============================================================== */
int readrgb(char *filename, struct xvimage ** r, struct xvimage ** g, struct xvimage ** b)
/* =============================================================== */
#undef F_NAME
#define F_NAME "readrgb"
{
  FILE *fd;
  struct RGBFILEHEADER FileHeader;
  uint8_t *R, *G, *B;
  int i, j, rs, cs, N;

#ifdef DOSIO
  fd = fopen(filename,"rb");
#endif
#ifdef UNIXIO
  fd = fopen(filename,"r");
#endif
  if (!fd)
  {
    fprintf(stderr, "%s: cannot open file: %s\n", F_NAME, filename);
    return 0;
  }

  freadushort(&(FileHeader.magic), fd);
  FileHeader.compression = (uint8_t)getc(fd);
  FileHeader.bytespercha = (uint8_t)getc(fd);
  freadushort(&(FileHeader.dim), fd);
  freadushort(&(FileHeader.width), fd);
  freadushort(&(FileHeader.height), fd);

  if (FileHeader.magic != 474)
  {
    fprintf(stderr, "bad rgb format magic number: 474 expected, %d found\n", 
            FileHeader.magic);
    return 0;
  }

  if (FileHeader.compression != 0)
  {
    fprintf(stderr, "restricted rgb format conversion:\n");
    fprintf(stderr, "compression tag must be 0 (No compression), found: %d\n", FileHeader.compression);
    return 0;
  }
  rs = FileHeader.width;
  cs = FileHeader.height;
  N = rs * cs;
  *r = allocimage(NULL, rs, cs, 1, VFF_TYP_1_BYTE); 
  *g = allocimage(NULL, rs, cs, 1, VFF_TYP_1_BYTE); 
  *b = allocimage(NULL, rs, cs, 1, VFF_TYP_1_BYTE); 
  if ((*r == NULL) || (*g == NULL) || (*b == NULL))
  {
    fprintf(stderr, "%s: allocimage failed\n", F_NAME);
    return 0;
  }
  R = ((UCHARDATA(*r)));
  G = ((UCHARDATA(*g)));
  B = ((UCHARDATA(*b)));

  for (i = 0; i < 502; i++)  /** padding bytes **/
    (void)getc(fd);

  for (j = cs-1; j >= 0 ; j--)
  for (i = 0; i < rs; i++)  /** red bytes **/
    R[(j*rs)+i] = (uint8_t)getc(fd);
  for (j = cs-1; j >= 0 ; j--)
  for (i = 0; i < rs; i++)  /** green bytes **/
    G[(j*rs)+i] = (uint8_t)getc(fd);
  for (j = cs-1; j >= 0 ; j--)
  for (i = 0; i < rs; i++)  /** blue bytes **/
    B[(j*rs)+i] = (uint8_t)getc(fd);
  fclose(fd);
  return 1;
} /* readrgb() */

#ifdef TEST
int main()
{
  struct xvimage *im = readimage("test1.pgm");
  struct xvimage *am;
  if (!im) exit(1);
  am = copyimage(im);
  printimage(am);
  writeimage(am, "test2.pgm");
  razimage(am);
  copy2image(im, am);
  writeimage(am, "test3.pgm");
}
#endif