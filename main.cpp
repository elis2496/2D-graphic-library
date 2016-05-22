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
//yutyu
struct BFH
{
    unsigned short bfType;           /* Magic number for file ( 0x4d42 | 0x4349 | 0x5450)*/
    unsigned int   bfSize;           /* Size of file */
    unsigned short bfReserved1;      /* Reserved */
    unsigned short bfReserved2;      /* ... */
    unsigned int   bfOffBits;        /* Offset to bitmap data(смещение до поля данных, обычно 54 = 16 + biSize )*/
} bh;
#pragma pack()
struct BIH
{
    unsigned int   biSize;           /* Size of info header(размер структуры в байтах) */
    int            biWidth;          /* Width of image */
    int            biHeight;         /* Height of image */
    unsigned short biPlanes;         /* Number of color planes (должно быть 1)*/
    unsigned short biBitCount;       /* Number of bits per pixel (0|1|4|8|16|24|32)*/
    unsigned int   biCompression;    /* Type of compression to use (RI_RGB | BI_RLE8 | BI_PNG| BI_BITFIELDS | BI_JPEG | BI_PNG реально используетс лишь BI_RGB)*/
    unsigned int   biSizeImage;      /* Size of image data (количество байт в поле данных, обычно устанавливается в 0)*/
    int            biXPelsPerMeter;  /* X pixels per meter (горизонтальное разрешение)*/
    int            biYPelsPerMeter;  /* Y pixels per meter (вертикальное разрешение)*/
    unsigned int   biClrUsed;        /* Number of colors used */
    unsigned int   biClrImportant;   /* Number of important colors (можно считать просто равным ную)*/
} bih;

RGBTRIPLE* loadBMP( const char *fname, int &mx, int &my ) //функция загрузки файла
{
    mx=my=-1;
    FILE *f = fopen( fname, "rb" );
    if(!f)return NULL;
    size_t res;
    res=fread(&bh, 1, sizeof(BFH), f );// читаем заголовок
    if(res!=sizeof(BFH))fclose(f); // проверяем сигнатуру
    if( bh.bfType!=0x4d42 && bh.bfType!=0x4349 && bh.bfType!=0x5450 )
    {
        fclose(f);
        return NULL;
    }
    res=fread( &bih, 1, sizeof(BIH), f );
    if(res!=sizeof(BIH))
    {
        fclose(f);    // проверка размера файла
        return NULL;
    }
    fseek(f, 0, SEEK_END);//к концу добавили ноль
    int filesize = ftell(f);//определяем текущую позицию в файле
    fseek( f, bh.bfOffBits, SEEK_SET);// восстановим указатель в файле:
// проверим условия
    if((bh.bfSize!=filesize)||(bh.bfReserved1!=0)||(bh.bfReserved2!=0)||(bih.biPlanes!=1)||(bih.biSize!=40 && bih.biSize!=108 && bih.biSize!=124)||(bh.bfOffBits!=14+bih.biSize)||(bih.biWidth <1)||(bih.biWidth >1000000)||(bih.biHeight<1)||(bih.biHeight>1000000)||(bih.biBitCount!= 24)/*рассматриваем только полноцветные изображения*/||(bih.biCompression!=0)/*рассматриваем только несжатие изображения*/)
    {
        fclose(f);
        return NULL;
    }
// Заголовок прочитан и проверен, тип - верный (BGR-24), размеры (mx,my) найдены
    mx = bih.biWidth;
    my = bih.biHeight;
// выделим память для результата
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
// длина каждой строки выровнена до кратного четырех
        while((z++ % 4) != 0)
        {
            res = fread( &tmpByte, 1, sizeof(byte), f );
        }
    }
    fclose(f);
    return v;
}
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

bool saveBMP(const char *fname, RGBTRIPLE *v, int mx, int my) //функция сохранения
{
    bih.biWidth=mx;
    bih.biHeight=my;
    FILE *f=fopen(fname, "wb");
    if(!f) return false;
    size_t res;
// пишем заголовок
//bih.biWidth=mx;
//bih.biHeight=my;
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
// восстановим указатель в файле:
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
        // длина каждой строки выровнена до кратного четырех
        while((z++ % 4)!= 0)
        {
            res = fwrite( &tmpByte, 1, sizeof(byte), f );
        }
    }
    fclose(f);
    return true; // OK
}
RGBTRIPLE* scale(const char *fname, RGBTRIPLE *v, int &mx, int &my, float zoom) //масштабирование
{
    int nmx, nmy;
    int k=0,k1=0;
    nmx=round(mx*zoom);
    nmy=round(my*zoom);
    RGBTRIPLE* v1 = new RGBTRIPLE[nmx*nmy];
    for(int y=0; y<my; y++)
    {
        int x=0;
        for(; x<mx; x++)
            for(k=round(y*zoom); k<round(y*zoom+zoom); k++)
                for(k1=round(x*zoom); k1<round(x*zoom+zoom); k1++)
                    *(v1+nmx*k+k1)=*(v+y*mx+x);
    }
    mx=round(mx*zoom);
    my=round(my*zoom);
    return v1;
}
RGBTRIPLE* ex(RGBTRIPLE *v, int &mx, int &my, float x1, float y1) //потянуть по х и у
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
RGBTRIPLE* rot(RGBTRIPLE *v, int &mx, int &my, int r)
{
    float rad=(2*3.14*r)/360;//  cosl -sinl
    float coss=(float)cos(rad);//sinl  cosl
    float sinn=(float)sin(rad);
    //my-height
    float p1x=(-my*sinn);
    float p1y=(my*coss);
    float p2x=(my*coss-my*sinn);
    float p2y=(my*coss+my*sinn);
    float p3x=(mx*coss);
    float p3y=(mx*sinn);

    float minx=min(0,min(p1x,min(p2x,p3x)));
    float miny=min(0,min(p1y,min(p2y,p3y)));
    float maxx=max(p1x,max(p2x,p3x));
    float maxy=max(p1y,max(p2y,p3y));
    int nmx=(int)ceil(maxx-minx);
    int nmy=(int)ceil(maxy-miny);
    RGBTRIPLE *v1=new RGBTRIPLE[nmx*nmy];
    int x,y;
    for(x=0; x<nmx; x++)
    {
        for(y=0; y<nmy; y++)
        {
            int nx=(int)((x+minx)*coss+(y+miny)*sinn);
            int ny=(int)((y+miny)*coss-(x+minx)*sinn);
            if(nx>=0&&nx<mx&&ny>=0&&ny<my)
            {
                *(v1+nmx*y+x)=*(v+mx*ny+nx);
            }
        }
    }
    mx=nmx;
    my=nmy;
    return v1;
}

RGBTRIPLE* matrixConvolution(RGBTRIPLE *v, int mx, int my, int matrix[3][3], int div) //маьрица свёртки
{
    RGBTRIPLE neighbors[3][3];
    RGBTRIPLE* v1 = new RGBTRIPLE[mx*my];
    RGBTRIPLE* ptr1 = v1;
    int y;
// для omp - #pragma omp parallel for private(y), private(neighbors)+
    for(y = 0; y < my; y++)
    {
        for(int x = 0; x < mx; x++)
        {
            int rgbtRed = 0, rgbtGreen = 0, rgbtBlue = 0;
            // вычисляем новое значение, используя соседей... учитываем, что строки идут снизу вверх!
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
                    rgbtRed += matrix[i][j]*neighbors[i][j].rgbtRed/div;
                    rgbtGreen += matrix[i][j]*neighbors[i][j].rgbtGreen/div;
                    rgbtBlue += matrix[i][j]*neighbors[i][j].rgbtBlue/div;
                }
            }
            RGBTRIPLE currentPixel = normalizeRGBT(rgbtRed, rgbtGreen, rgbtBlue);
            *(ptr1+mx*y+x) = currentPixel;
        }
    }
    return v1;
}
RGBTRIPLE* matrixConvolution1(RGBTRIPLE *v, int mx, int my, double matrix[3][3], int div) //маьрица свёртки
{
    RGBTRIPLE neighbors[3][3];
    RGBTRIPLE* v1 = new RGBTRIPLE[mx*my];
    RGBTRIPLE* ptr1 = v1;
    int y;
// для omp - #pragma omp parallel for private(y), private(neighbors)+
    for(y = 0; y < my; y++)
    {
        for(int x = 0; x < mx; x++)
        {
            int rgbtRed = 0, rgbtGreen = 0, rgbtBlue = 0;
            // вычисляем новое значение, используя соседей... учитываем, что строки идут снизу вверх!
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
RGBTRIPLE* ChoseOp(RGBTRIPLE *v, int &mx, int &my)
{
    printf("Enter:\n");
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
    printf("11 to exit\n");
    int n;
    RGBTRIPLE *v1;
    scanf("%d",&n);
    switch (n){
        case 1:
            {int matrix1[3][3] = { {1, 2, 1 },
                                 {2, 4, 2 },
                                 {1, 2, 1 }
                               };
            v1 = matrixConvolution(v, mx, my, matrix1, 16);
            break;
            }
        case 2:
            {int matrix2[3][3] = { {-1, -1, -1 },
                                 {-1, 9, -1 },
                                 {-1, -1, -1}
                               };
            v1 = matrixConvolution(v, mx, my, matrix2, 1);
            break;
            }
        case 3:
            {double matrix3[3][3] = { {-0.1, 0.2, -0.1 },
                                 {0.2, 3, 0.2},
                                 {-0.1, 0.2, 0.1}
                               };
            v1 = matrixConvolution1(v, mx, my, matrix3, 3);
            break;
            }
        case 4:
            {double matrix3[3][3] = { {-0.1, 0.1, -0.1 },
                                 {0.1, 0.5, 0.1},
                                 {-0.1, 0.1, 0.1}
                               };
            v1 = matrixConvolution1(v, mx, my, matrix3, 1);
            break;
            }
        case 5:
             {printf("Enter elements of matrix");
             int i,j,div=0;
             int matrix[3][3];
             for(i=0;i<3;i++)
                for(j=0;j<3;j++)
                  {
                      scanf("%d",matrix[i][j]);
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
            {printf("Enter angle\n");
            int r;
            scanf("%d",&r);
            v1 = rot(v,mx,my,r);
            break;
            }
        case 11:
            break;


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
    const char filename[] = "image1.bmp";
    int mx, my;
    RGBTRIPLE *im = loadBMP(filename, mx, my);
    const char filename2[] = "image2.bmp";
    RGBTRIPLE *v;
    int f=1;
    int mmx=mx;
    int mmy=my;
    while(f)
    {
        v=ChoseOp(im,mx,my);
        if((mmx==mx)&&(mmy==my))cpy(im,v,mx,my);
          else {
                im=(RGBTRIPLE*)realloc(im,sizeof(RGBTRIPLE)*my*mx);
                cpy(im,v,mx,my);
                mmx=mx;
                mmy=my;
               }
        printf("if you want to stop enter 0\n");
        scanf("%d",&f);
        printf("\n");
    }
    if(saveBMP(filename2,im,mx, my))printf("Image successfully created");
      else printf ("Couldn't create\n");
}


