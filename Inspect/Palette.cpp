//#include <algorithm>
#include "Palette.h"

CPalette::CPalette()
{
	m_exists = false;
	m_table = NULL;
	m_palette = NULL;
}

CPalette::~CPalette(void)
{
}
int CPalette::Create(cv::Mat& src, int size)	// must overload
{
	m_PaletteSize = size>256 ? 256 : size;
	if(m_table != NULL)
	{
		delete m_table;
	}
	m_table = new ColorData[size];
	if(m_palette != NULL)
	{
		delete m_palette;
	}
	m_palette = new BGRType[size];
	m_exists = true;
	return 0;
}
int CPalette::Palettize(cv::Mat& src, cv::Mat& dst)
{
	if((dst.rows != src.rows) || (dst.cols != src.cols) || (dst.type() != CV_8UC1) )
	{	// re-allocate destination
		dst.create(src.rows,src.cols,CV_8UC1);
	}
	int height, width;
	height = src.rows; width = src.cols;

	BGRType color;
	unsigned char *pSrc, *pDst;
	int i,j;
	for(i=0; i<height; i++)
	{
		pSrc = (unsigned char*)src.ptr(i);
		pDst = (unsigned char*)dst.ptr(i);
		for(j=0; j<width; j++)
		{
			color.b = pSrc[3*j]; color.g = pSrc[3*j+1]; color.r = pSrc[3*j+2];
			pDst[j] = QuantizeColor(color);
		}
	}
	return 0;	// OK
}

#if(0)
	// get zscore for color relative to its palette brothers
int CPalette::ZScore(cv::Mat& PalImg, cv::Mat& Img, cv::Mat& ZImg)
{
	int width = PalImg.cols;
	int height = PalImg.rows;
	unsigned char *pPal;
	unsigned char *pColor;
	float *pZ;
	ColorData PalColor;
	int step = Img.channels();
	for(int i=0; i<height; i++)
	{
		pPal   = (unsigned char*)PalImg.ptr(i);
		pColor = (unsigned char*)Img.ptr(i);
		pZ     = (float*)ZImg.ptr(i);
		for(int j=0; j<width; j++)
		{
			PalColor = m_table[pPal[j]];
			pZ[j] =  abs(((float)pColor[j*step  ]-PalColor.color.b)/PalColor.sigmaB)
				   + abs(((float)pColor[j*step+1]-PalColor.color.g)/PalColor.sigmaG)
				   + abs(((float)pColor[j*step+2]-PalColor.color.r)/PalColor.sigmaR);
		}
	}
	return 0;	// OK
}

static float sigma(long s, long s2, int N)
{
	float sigma = sqrt((float)N * s2 - s*s)/N;
	return sigma;
}
#endif
void CPalette::DePalettize(cv::Mat& src, cv::Mat& dst)
{
	if((dst.rows != src.rows) || (dst.cols != src.cols) || (dst.type() != CV_8UC3) )
	{	// re-allocate destination
		dst.create(src.rows,src.cols,CV_8UC3);
	}
	BGRType color;
	unsigned char *pSrc, *pDst;
	int height, width;
	height = src.rows; width = src.cols;
	int i,j;
	for(i=0; i<height; i++)
	{
		pSrc = src.ptr(i);
		pDst = dst.ptr(i);
		for(j=0; j<width; j++)
		{
			color = m_palette[pSrc[j]];
			pDst[3*j] = color.b;
			pDst[3*j+1] = color.g;
			pDst[3*j+2] = color.r;
		}
	}

}

static int encput(int byt, int cnt, FILE *fid);
static int encline(unsigned char *line, int length, FILE *fp);

typedef struct {
	char        manuf;              /* Program manufacturer ID          */
    char        vers;               /* Version number                   */
	char        rle;                /* if 1 then image run-length enc   */
    char        bitpx;              /* Bits per pixel                   */
	unsigned short    x1,           /* Image bounds                     */
                y1,
				x2,
                y2,
				hres,               /* Horizontal resolution            */
                vres;               /* vertical resolution              */
	unsigned char rgb[48];          /* Palette                          */
    char        vmode;              /* Unused                           */
	char        nplanes;            /* Number of planes                 */
    unsigned short   bpline;        /* bytes per line                   */
	unsigned short   palinfo;       /* 1=color, 2=grayscale             */
    unsigned short   hscreen;
	unsigned short   vscreen;
    char        xtra[54];           /* Future use?                      */
} PCXhdr;

typedef struct {
	unsigned char r;
    unsigned char g;
	unsigned char b;
} PCXCOLOR;

/****************************************************************************
**
** SaveAsPCX
**
** Write a PCX file version of the image
*/
int CPalette::SavePalettizedAsPCX( cv::Mat& image,	// matrix is Palettized pixels
			  char* pcxfile
			  )
{
	int stat = 0;
	double nrgbr = 63, nrgbg =63, nrgbb = 63;
	int resx = image.cols;
	int resy = image.rows;
	int npal = m_PaletteSize;
	ColorData *colortable = m_table;
//	COctreeNode *octree = m_pOctree;
    PCXhdr          hdr;
//    unsigned char   *buf;
	PCXCOLOR        *pal;
    int             i,k;
//	RGBType			color;
	unsigned char *pPix;

	FILE *pf;
	fopen_s(&pf,pcxfile, "wb");
	if (pf == NULL)
	{
        // fprintf(stderr, "Can't open file %s for writing\n", pcxfile);
		return 1;
    }

    /*
	** Set up the PCX Palettte.
    */
	pal = (PCXCOLOR *)malloc(256*sizeof(PCXCOLOR));
	if(!pal)
	{
		stat = 2;
		goto ESCAPE2;
	}
	for (k = 0; k < npal ; k++)
	{
		pal[k].r = (unsigned char)(colortable[k].color.r);
		pal[k].g = (unsigned char)(colortable[k].color.g);
		pal[k].b = (unsigned char)(colortable[k].color.b);
	}

    /*
	** Fill in the PCX header
    */
	hdr.manuf = (char)10;
    hdr.vers = (char)5;
	hdr.rle = (char)1;
    hdr.bitpx = 8;
	hdr.x1 = hdr.y1 = 0;
	hdr.x2 = resx - 1;
	hdr.y2 = resy - 1;
	hdr.hres = 640;
	hdr.vres = 480;
	hdr.hscreen = 0;
	hdr.vscreen = 0;
    hdr.bpline = hdr.x2;
	hdr.vmode = (char)0;
    hdr.nplanes = 1;
	hdr.palinfo = 1;
    memset(hdr.xtra,0x00,54);

    fwrite(&hdr, sizeof(PCXhdr), 1, pf);

    /*
	** Scan the image and pack pixels for the PCX image
    */

	for (i = 0; i < resy; i++)
	{
		pPix = image.ptr(i);
		/*
		** Encode this scan line and write it to the file.  If return is non-0
		** then the write failed
		*/
		if (encline(pPix, hdr.x2, pf))
		{
			stat = 3;
			goto ESCAPE1;
		}
	}
	/*
	** Store the palette
	*/
#if(0)
	for (i = 0; i < 256; i++)
	{
        pal[i].r *= 4;
        pal[i].g *= 4;
        pal[i].b *= 4;
	}
#endif
	putc(0x0c, pf);
	fwrite(pal, 768, 1, pf);
ESCAPE1:
	free(pal);
ESCAPE2:
	fclose(pf);
	return stat;
}

/***************************************************************************
**
** encline
**
** Encode a line for RLE PCX image data
*/
static int encline(unsigned char *line, int length, FILE *fp)
{
	int             local, last;
    int             srcindex;
	int             runcount;
    int             total;

    last = *line++;
	runcount = 1;
    srcindex = 1;
	total = 0;
    while (srcindex < length) {
		local = *line++;
        srcindex++;
		if (local == last) {
            runcount++;
			if (runcount == 0x3f) {
                if (encput(last, runcount, fp)) {
					return 1;
                }
				total += runcount;
                runcount = 0;
			}
        }
		else {
            if (runcount) {
				if (encput(last,runcount,fp)) {
                    return 1;
				}
                total += runcount;
			}
            last = local;
			runcount = 1;
        }
	}
    if (runcount) {
		if (encput(last, runcount, fp)) {
            return 1;
		}
        total += runcount;
	}
    return 0;
}


static int encput(int byt, int cnt, FILE *fid)
{
    if (cnt) {
		if (((byt & 0x00c0) == 0x00c0) || (cnt > 1)) {
            if (putc(0xc0 | cnt, fid) == EOF) {
				return 1;
            }
		}
        if (putc(byt, fid) == EOF) {
			return 1;
        }
	}
    return 0;
}

inline RGBType BGR2RGB(BGRType bgr)
{
	RGBType rgb;
	rgb.r=bgr.r; rgb.g=bgr.g; rgb.b=bgr.b;
	return rgb;
}
inline BGRType RGB2BGR(RGBType rgb)
{
	BGRType bgr;
	bgr.r=rgb.r; bgr.g=rgb.g; bgr.b=rgb.b;
	return bgr;
}
