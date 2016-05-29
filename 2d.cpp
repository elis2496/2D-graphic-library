#include <stdio.h>
#include <tchar.h>
#include <iostream>
#include <iomanip>
#include <fstream>
#include <cstdlib>
#include <ctime>
#include <cassert>
#include <math.h>
#include <windows.h>
#pragma pack(2)
#define min(a, b) (((a) < (b)) ? (a) : (b))
#define max(a, b) (((a) > (b)) ? (a) : (b))
struct BFH
{
    unsigned short bfType;           /* Magic number for file ( 0x4d42 | 0x4349 | 0x5450)*/
    unsigned int   bfSize;           /* Size of file */
    unsigned short bfReserved1;      /* Reserved */
    unsigned short bfReserved2;      /* ... */
    unsigned int   bfOffBits;        /* Offset to bitmap data(54 = 16 + biSize )*/
} bh;
#pragma pack()
struct BIH
{
    unsigned int   biSize;           /* Size of info header */
    int            biWidth;          /* Width of image */
    int            biHeight;         /* Height of image */
    unsigned short biPlanes;         /* Number of color planes (must be 1)*/
    unsigned short biBitCount;       /* Number of bits per pixel (0|1|4|8|16|24|32)*/
    unsigned int   biCompression;    /* Type of compression to use (RI_RGB | BI_RLE8 | BI_PNG| BI_BITFIELDS | BI_JPEG | BI_PNG реально используетс лишь BI_RGB)*/
    unsigned int   biSizeImage;      /* Size of image data (usually 0)*/
    int            biXPelsPerMeter;  /* X pixels per meter */
    int            biYPelsPerMeter;  /* Y pixels per meter */
    unsigned int   biClrUsed;        /* Number of colors used */
    unsigned int   biClrImportant;   /* Number of important colors (we can just write 0)*/
} bih;
/*!Function for load file
*/
RGBTRIPLE* loadBMP( const char *fname, int &mx, int &my )
{
    mx=my=-1;
    FILE *f = fopen( fname, "rb" );
    if(!f)return NULL;
    size_t res;
    res=fread(&bh, 1, sizeof(BFH), f );
    if(res!=sizeof(BFH)){printf("!\n");fclose(f); }
    if( bh.bfType!=0x4d42 && bh.bfType!=0x4349 && bh.bfType!=0x5450 )
    {
        printf("!!\n");
        fclose(f);
        return NULL;
    }
    res=fread( &bih, 1, sizeof(BIH), f );
    if(res!=sizeof(BIH))
    {
        printf("!\n");
        fclose(f);
        return NULL;
    }
    fseek(f, 0, SEEK_END);
    int filesize = ftell(f);
    fseek( f, bh.bfOffBits, SEEK_SET);
    bh.bfSize=filesize;
    if(bh.bfSize!=filesize){printf("bh.bfSize!=filesize\n");fclose(f);return 0;}
    if(bh.bfReserved1!=0){printf("bh.bfReserved1!=0");fclose(f);return 0;}
    if(bh.bfReserved2!=0){printf("bh.bfReserved2!=0");fclose(f);return 0;}
    if(bih.biPlanes!=1){printf("bih.biPlanes!=1");fclose(f);return 0;}
    if(bih.biSize!=40 && bih.biSize!=108 && bih.biSize!=124){printf("bih.biSize!=40 && bih.biSize!=108 && bih.biSize!=124");fclose(f);return 0;}
    if(bh.bfOffBits!=14+bih.biSize){printf("bh.bfOffBits!=14+bih.biSize");fclose(f);return 0;}
    if(bih.biWidth >1000000){printf("bih.biWidth >1000000");fclose(f);return 0;}
    if(bih.biHeight<1){printf("bih.biHeight<1");fclose(f);return 0;}
    if(bih.biHeight>1000000){printf("bih.biHeight>1000000");fclose(f);return 0;}
    if(bih.biBitCount!= 24){printf("bih.biBitCount!= 24");fclose(f);return 0;}
    if(bih.biCompression!=0){printf("bih.biCompression!=0");fclose(f);return 0;}
    mx = bih.biWidth;
    my = bih.biHeight;
    RGBTRIPLE* v = new RGBTRIPLE[mx*my];
    RGBTRIPLE tmp;
    byte tmpByte;
    RGBTRIPLE *ptr1 = v;
    for(int y=0; y<my; y++)
    {
        int x = 0;
        for(; x<mx; x++)
        {
            res = fread( &tmp, 1, sizeof(RGBTRIPLE), f);
            *ptr1++ = tmp;
        }
        int z = x*sizeof(RGBTRIPLE);
        while((z++ % 4) != 0)
        {
            res = fread( &tmpByte, 1, sizeof(byte), f );
        }
    }
    fclose(f);
    return v;
}
/*!Function for normalize RGB
*/
RGBTRIPLE normalizeRGBT(int rgbtRed, int rgbtGreen, int rgbtBlue)
{
    if(rgbtRed<0)rgbtRed=0;
    if(rgbtGreen<0)rgbtGreen=0;
    if(rgbtBlue<0)rgbtBlue=0;
    if(rgbtRed>255)rgbtRed=255;
    if(rgbtGreen>255)rgbtGreen=255;
    if(rgbtBlue>255)rgbtBlue=255;
    RGBTRIPLE currentPixel;
    currentPixel.rgbtRed = rgbtRed;
    currentPixel.rgbtGreen = rgbtGreen;
    currentPixel.rgbtBlue = rgbtBlue;
    return currentPixel;
}
/*!Function for save file
*/
bool saveBMP(const char *fname, RGBTRIPLE *v, int mx, int my) //
{
    bih.biWidth=mx;
    bih.biHeight=my;
    FILE *f=fopen(fname, "wb");
    if(!f) return false;
    size_t res;
    res=fwrite(&bh,1,sizeof(BFH),f);
    if(res!=sizeof(BFH))
    {
        fclose(f);
        return false;
    }
    res=fwrite(&bih,1,sizeof(BIH),f);
    if(res!=sizeof(BIH))
    {
        fclose(f);
        return false;
    }
    fseek(f, bh.bfOffBits, SEEK_SET);
    RGBTRIPLE tmp;
    byte tmpByte;
    RGBTRIPLE *ptr1 = v;
    for(int y=0; y<my; y++)
    {
        int x=0;
        for(; x<mx; x++)
        {
            tmp = *ptr1;
            res = fwrite( &tmp, 1, sizeof(RGBTRIPLE), f );
            *(ptr1++);
        }
        int z=x*sizeof(RGBTRIPLE);
        while((z++ % 4)!= 0)
        {
            res = fwrite( &tmpByte, 1, sizeof(byte), f );
        }
    }
    fclose(f);
    return true;
}
/*!
Increases or decreases the image
\param v An array of pixels
\param mx, my  The length and width of the image
\param x1,y1 How many times to zoom in / zoom out on the x and y respectively,
\return New array of pixels

Each pixel we associate x1 * y1 pixels
*/
RGBTRIPLE* ex(RGBTRIPLE *v, int &mx, int &my, float x1, float y1)
{
    int nmx, nmy;
    int k=0,k1=0;
    nmx=round(mx*x1);
    nmy=round(my*y1);
    RGBTRIPLE* v1 = new RGBTRIPLE[nmx*nmy];
    for(int y=0; y<my; y++)
    {
        int x=0;
        for(; x<mx; x++)
            for(k=round(y*y1); k<round(y*y1+y1); k++)
                for(k1=round(x*x1); k1<round(x*x1+x1); k1++)
                    *(v1+nmx*k+k1)=*(v+y*mx+x);
    }
    mx=round(mx*x1);
    my=round(my*y1);
    return v1;
}
/*!
It reflects the image vertically
\param v An array of pixels
\param mx, my The length and width of the image
\return New array of pixels
*/
RGBTRIPLE* reflectionY(RGBTRIPLE *v, int mx, int my)
{
    RGBTRIPLE* v1 = new RGBTRIPLE[mx*my];
    for(int y=0; y<my; y++)
    {
        int x=0;
        for(; x<mx; x++)
            *(v1+mx*y+x)=*(v+mx*y+mx-x);
    }

    return v1;

}
/*!
It reflects the image horizontally
\param v An array of pixels
\param mx, my The length and width of the image
\return New array of pixels
*/
RGBTRIPLE* reflectionX(RGBTRIPLE *v, int mx, int my)
{
    RGBTRIPLE* v1 = new RGBTRIPLE[mx*my];
    for(int y=0; y<my; y++)
    {
        int x=0;
        for(; x<mx; x++)
            *(v1+mx*y+x)=*(v+mx*(my-y-1)+x);
    }

    return v1;

}
/*!
It reflects the image horizontally and vertically
\param v An array of pixels
\param mx, my The length and width of the image
\return New array of pixels
*/
RGBTRIPLE* reflectionXY(RGBTRIPLE *v, int mx, int my)
{
    RGBTRIPLE* v1 = new RGBTRIPLE[mx*my];
    for(int y=0; y<my; y++)
    {
        int x=0;
        for(; x<mx; x++)
            *(v1+mx*y+x)=*(v+mx*(my-y-1)+mx-x);
     }

    return v1;

}

/*!
Image filter
\param v Array of pixels
\param mx, my  The length and width of the image
\param matrix Convolution matrix
\param div Normalization coefficient (usually equal to the sum of all matrix elements)
\return New array of pixels

We choose from the general array of pixels array of 3 by 3 and multiply it by the matrix convolution.
*/

RGBTRIPLE* matrixConvolution(RGBTRIPLE *v, int mx, int my, double matrix[3][3], int div)
{
    RGBTRIPLE neighbors[3][3];
    RGBTRIPLE* v1 = new RGBTRIPLE[mx*my];
    RGBTRIPLE* ptr1 = v1;
    int y;
    for(y = 0; y < my; y++)
    {
        for(int x = 0; x < mx; x++)
        {
            int rgbtRed = 0, rgbtGreen = 0, rgbtBlue = 0;
            neighbors[1][1] = *(v + y*mx + x);
            neighbors[0][1] = (y != my-1) ? *(v + (y+1)*mx + x) : neighbors[1][1];
            neighbors[1][0] = (x != 0) ? *(v + y*mx + x-1) : neighbors[1][1];
            neighbors[1][2] = (x != mx-1) ? *(v + y*mx + x+1) : neighbors[1][1];
            neighbors[2][1] = (y != 0) ? *(v + (y-1)*mx + x) : neighbors[1][1];
            neighbors[0][0] = (x == 0) ? neighbors[0][1] : ( (y == my-1) ? neighbors[1][0] : *(v + (y+1)*mx + x-1) );
            neighbors[0][2] = (x == mx-1) ? neighbors[0][1] : ( (y == my-1) ? neighbors[1][2] : *(v + (y+1)*mx + x+1) );
            neighbors[2][0] = (x == 0) ? neighbors[2][1] : ( (y == 0) ? neighbors[1][0] : *(v + (y-1)*mx + x-1) );
            neighbors[2][2] = (x == mx-1) ? neighbors[2][1] : ( (y == 0) ? neighbors[1][2] : *(v + (y-1)*mx + x+1) );
            if((x == 0) && (y == my-1))
            {
                rgbtRed = (neighbors[1][0].rgbtRed + neighbors[0][1].rgbtRed)/2;
                rgbtGreen = (neighbors[1][0].rgbtGreen + neighbors[0][1].rgbtGreen)/2;
                rgbtBlue = (neighbors[1][0].rgbtBlue + neighbors[0][1].rgbtBlue)/2;
                neighbors[0][0] = normalizeRGBT(rgbtRed, rgbtGreen, rgbtBlue);
            }
            if((x == 0) && (y == 0))
            {
                rgbtRed = (neighbors[1][0].rgbtRed + neighbors[2][1].rgbtRed)/2;
                rgbtGreen = (neighbors[1][0].rgbtGreen + neighbors[2][1].rgbtGreen)/2;
                rgbtBlue = (neighbors[1][0].rgbtBlue + neighbors[2][1].rgbtBlue)/2;
                neighbors[2][0] = normalizeRGBT(rgbtRed, rgbtGreen, rgbtBlue);
            }
            if((x == mx-1) && (y == my-1))
            {
                rgbtRed = (neighbors[1][2].rgbtRed + neighbors[0][1].rgbtRed)/2;
                rgbtGreen = (neighbors[1][2].rgbtGreen + neighbors[0][1].rgbtGreen)/2;
                rgbtBlue = (neighbors[1][2].rgbtBlue + neighbors[0][1].rgbtBlue)/2;
                neighbors[0][2] = normalizeRGBT(rgbtRed, rgbtGreen, rgbtBlue);
            }
            if((x == mx-1) && (y == 0))
            {
                rgbtRed = (neighbors[1][2].rgbtRed + neighbors[2][1].rgbtRed)/2;
                rgbtGreen = (neighbors[1][2].rgbtGreen + neighbors[2][1].rgbtGreen)/2;
                rgbtBlue = (neighbors[1][2].rgbtBlue + neighbors[2][1].rgbtBlue)/2;
                neighbors[2][2] = normalizeRGBT(rgbtRed, rgbtGreen, rgbtBlue);
            }
            rgbtRed = 0, rgbtGreen = 0, rgbtBlue = 0;
            for(int i = 0; i < 3; ++i)
            {
                for(int j = 0; j < 3; ++j)
                {
                    rgbtRed += round(matrix[i][j]*neighbors[i][j].rgbtRed/div);
                    rgbtGreen += round(matrix[i][j]*neighbors[i][j].rgbtGreen/div);
                    rgbtBlue += round(matrix[i][j]*neighbors[i][j].rgbtBlue/div);
                }
            }
            RGBTRIPLE currentPixel = normalizeRGBT(rgbtRed, rgbtGreen, rgbtBlue);
            *(ptr1+mx*y+x) = currentPixel;
        }
    }
    return v1;
}
/*!
It rotates the image by a angle 90
\param v An array of pixels
\param mx, my  The length and width of the image
\return New array of pixels

*/
RGBTRIPLE* rot(RGBTRIPLE *v, int &mx, int &my)
{
    int nmx=my;
    int nmy=mx;
    RGBTRIPLE *v1=new RGBTRIPLE[nmx*nmy];
    int x,y;
    for(x=0; x<nmx; x++)
    {
        for(y=0; y<nmy; y++)
        {
                *(v1+nmx*y+x)=*(v+mx*(-x+my-1)+y);
        }
    }
    mx=nmx;
    my=nmy;
    return v1;
}
/*!This function I used to check work of library
Here we chose operations for our picture
*/
RGBTRIPLE* ChoseOp(RGBTRIPLE *v, int &mx, int &my, int &n)
{
    printf("Enter:\n");
    printf("0 to stop and exit\n");
    printf("1 to apply a blur filter\n");
    printf("2 to apply a filter sharpness\n");
    printf("3 to to increase brightness\n");
    printf("4 to darken the image\n");
    printf("5 to apply yours filter\n");
    printf("6 to reflect on the x\n");
    printf("7 to reflect on the y\n");
    printf("8 to reflect on the x and y\n");
    printf("9 to zoom\n");
    printf("10 to rotate the picture\n");
    RGBTRIPLE *v1;
    scanf("%d",&n);
    switch (n){
        case 1:
            {double matrix1[3][3] = { {1.0, 2.0, 1.0 },
                                 {2.0, 4.0, 2.0 },
                                 {1.0, 2.0, 1.0 }
                               };
            v1 = matrixConvolution(v, mx, my, matrix1, 16);
            break;
            }
        case 2:
            {double matrix2[3][3] = { {-1.0, -1.0, -1.0 },
                                 {-1.0, 9.0, -1.0 },
                                 {-1.0, -1.0, -1.0}
                               };
            v1 = matrixConvolution(v, mx, my, matrix2, 1);
            break;
            }
        case 3:
            {double matrix3[3][3] = { {-0.1, 0.2, -0.1 },
                                 {0.2, 3, 0.2},
                                 {-0.1, 0.2, 0.1}
                               };
            v1 = matrixConvolution(v, mx, my, matrix3, 3);
            break;
            }
        case 4:
            {double matrix3[3][3] = { {-0.1, 0.1, -0.1 },
                                 {0.1, 0.5, 0.1},
                                 {-0.1, 0.1, 0.1}
                               };
            v1 = matrixConvolution(v, mx, my, matrix3, 1);
            break;
            }
        case 5:
             {printf("Enter elements of matrix");
             int i,j,div=0;
             double matrix[3][3];
             for(i=0;i<3;i++)
                for(j=0;j<3;j++)
                  {
                      scanf("%lf",&matrix[i][j]);
                      printf("%lf",matrix[i][j]);
                      printf("\n");
                      div+=matrix[i][j];
                  }
            v1 = matrixConvolution(v, mx, my, matrix, div);
            break;
             }
        case 6:
            {v1 = reflectionX(v,mx,my);
            break;
            }
        case 7:
            {v1 = reflectionY(v,mx,my);
            break;
            }
        case 8:
            {v1 = reflectionXY(v,mx,my);
            break;
            }
        case 9:
            {printf("Enter zoom x\n");
            float x1;
            scanf("%f",&x1);
            printf("Enter zoom y\n");
            float y1;
            scanf("%f",&y1);
            v1 = ex(v, mx, my,x1,y1);
            break;
            }
        case 10:
            {
            v1 = rot(v,mx,my);
            break;
            }

    }
    return v1;
}

void cpy(RGBTRIPLE *v1, RGBTRIPLE *v2, int &mx, int &my)
{
    int y;
    for(y = 0; y < my; y++)
    {
        for(int x = 0; x < mx; x++)
        {
            *(v1+mx*y+x)=*(v2+mx*y+x);
        }
    }
}
int main()
{
    const char filename[] = "image11.bmp";
    int mx, my;
    RGBTRIPLE *im = loadBMP(filename, mx, my);
    RGBTRIPLE *v;
    int n=1;
    int mmx=mx;
    int mmy=my;
    while(n)
    {
        v=ChoseOp(im,mx,my,n);
        if((mmx==mx)&&(mmy==my))cpy(im,v,mx,my);
          else {
                im=(RGBTRIPLE*)realloc(im,sizeof(RGBTRIPLE)*my*mx);
                cpy(im,v,mx,my);
                mmx=mx;
                mmy=my;
               }

    }
    if(saveBMP(filename,im,mx, my))printf("Image successfully created");
      else printf ("Couldn't create\n");
}


