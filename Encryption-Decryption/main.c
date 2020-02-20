#include <stdio.h>
#include <stdlib.h>
#include <math.h>
void Dimensiuni_img(char *cale_imagine,int *w,int *h)
{
    int x;
    FILE *f=fopen(cale_imagine,"rb");
    if(f==NULL)
    {
        printf("Eroare!");
        return;
    }
    fseek(f,18,SEEK_SET);
    fread(&x,1,sizeof(int),f);*w=x;
    fread(&x,1,sizeof(int),f);*h=x;
    fclose(f);
}
void Secret_Key(unsigned int *R0,unsigned int *SV)
{
    FILE *f=fopen("secret_key.txt","r");
    if(f==NULL)
    {
        printf("Eroare la deschiderea fisierului ce contine cheia secreta!");
        return;
    }
    fscanf(f,"%u%u",R0,SV);
}
void Header(char *cale_imagine,char *cale_header)
{
    FILE *fin=fopen(cale_imagine,"rb");
    if(fin==NULL)
    {
        printf("Eroare1!");
        return;
    }
    FILE *fout=fopen(cale_header,"wb");
    if(fout==NULL)
    {
        printf("Eroare!2");
        return;
    }
    char x;int i;
    for(i=0;i<54;i++)
    {
        fread(&x,1,1,fin);
        fwrite(&x,1,1,fout);
        fflush(fout);
    }
    fclose(fin);fclose(fout);
}
int *Liniarizare(char *cale_imagine)
{
    int w,h,dim,padding=0,i,j,x,*L,n=0;
    FILE *f=fopen(cale_imagine,"rb");
    if(f==NULL)
    {
        printf("Eroare3!");
        return;
    }
    fseek(f,2,SEEK_SET);
    fread(&dim,sizeof(int),1,f);
    printf("Dimensiunea imaginii in octeti: %d",dim);
    fseek(f,18,SEEK_SET);
    fread(&w,sizeof(int),1,f);
    fread(&h,sizeof(int),1,f);
    printf("\nDimensiunea in pixeli: %d x %d",w,h);
    //Octetii de padding
    if(w%4!=0)
        padding=4-(w*3)%4;
    printf("\nOcteti de padding: %d\n",padding);
    fseek(f,54,SEEK_SET);
    L=(int *)malloc(w*h*sizeof(int));
    for(j=0;j<w;j++)
        {
            n=h*(w-j-1);
            for(i=0;i<h;i++)
            {
                fread(&x,1,3,f);
                L[n]=x;n++;
            }
            fseek(f,padding,SEEK_CUR);
        }
    fclose(f);
    return L;
}
void Salvare_memExt(char *cale_imagine,int *L,char *header,int w,int h)
{
    FILE *fin=fopen(header,"rb");
    if(fin==NULL)
    {
        printf("Eroare!4");
        return;
    }
    FILE *fout=fopen(cale_imagine,"wb");
    if(fout==NULL)
    {
        printf("Eroare!5");
        return;
    }
    int padding=0,k;
    if(w%4!=0)
        padding=(4-(3*w)%4)%4;
    char x,a=0;
    while(fread(&x,1,1,fin)==1)
        fwrite(&x,1,1,fout);
    int i,j,n;
    for(j=0;j<w;j++)
        {
            n=h*(w-j-1);
            for(i=0;i<h;i++)
            {
                fwrite(&L[n],1,3,fout);
                n++;
                fflush(fout);
            }
            for(k=0;k<padding;k++)
                fwrite(&a,1,1,fout);
        }
    fclose(fin);fclose(fout);
}
unsigned int XORSHIFT32(unsigned int R0)
{
    unsigned int x=R0;
    x^=x<<13;
    x^=x>>17;
    x^=x<<5;
    return x;
}
unsigned int *Durstenfeld(int w,int h,unsigned int *R)
{
    unsigned int *v=(unsigned int *)malloc(w*h*sizeof(unsigned int));
    unsigned int k,i,aux,r;
    for(i=0;i<w*h;i++)
        v[i]=i;
    i=1;
    for(k=w*h-1;k>=1;k--)
    {
        r=R[i]%(k+1);
        aux=v[k];
        v[k]=v[r];
        v[r]=aux;i++;
    }
    return v;
}
void Criptare (char *imag_init,char *imag_cript,char *sec_k)
{
    int *L,w,h;
    Dimensiuni_img(imag_init,&w,&h);
    printf("%d %d\n",w,h);
    Header(imag_init,"header.bin");
    L=Liniarizare(imag_init);
    Salvare_memExt("imagine.bmp",L,"header.bin",w,h);//printf("\nPixelul 1: %d\n",L[0]);
    unsigned int i,R0,SV,*R;
    R=(unsigned int *)malloc((2*w*h-1)*sizeof(unsigned int));
    Secret_Key(&R0,&SV);//printf("\n%u %u",R0,SV);
    R[1]=XORSHIFT32(R0);
    for(i=2;i<2*w*h;i++)
        R[i]=XORSHIFT32(R[i-1]);
    unsigned int *v;v=Durstenfeld(w,h,R);
    unsigned int *L1=(unsigned int*)malloc(w*h*sizeof(unsigned int));
    for(i=0;i<w*h;i++)
        L1[v[i]]=L[i];
    Salvare_memExt("imagine_salvata.bmp",L1,"header.bin",w,h);
    unsigned int *C=(unsigned int*)malloc(w*h*sizeof(unsigned int));
    C[0]=SV^L1[0]^R[w*h];
    for(i=1;i<w*h;i++)
        C[i]=C[i-1]^L1[i]^R[w*h+i];
    Salvare_memExt(imag_cript,C,"header.bin",w,h);
    free(R);free(v);free(L);free(L1);free(C);
}

void Decriptare(char *imag_cript,char *imag_decript,char *sec_k)
{
    int w,h;
    unsigned int *C;
    Dimensiuni_img(imag_cript,&w,&h);
    Header(imag_cript,"header.bin");
    C=Liniarizare(imag_cript);
    unsigned int i,R0,SV,*R;
    R=(unsigned int *)malloc(2*w*h*sizeof(unsigned int));
    Secret_Key(&R0,&SV);//printf("\n%u %u",R0,SV);
    R[1]=XORSHIFT32(R0);
    for(i=2;i<2*w*h;i++)
        R[i]=XORSHIFT32(R[i-1]);
    unsigned int *v;v=Durstenfeld(w,h,R);
    unsigned int *v1=(unsigned int *)malloc(w*h*sizeof(unsigned int));
    for(i=0;i<w*h;i++)
        v1[v[i]]=i;
    unsigned int *C1=(unsigned int *)malloc(w*h*sizeof(unsigned int));
    C1[0]=SV^C[0]^R[w*h];
    for(i=1;i<w*h;i++)
        C1[i]=C[i-1]^C[i]^R[w*h+i];
    unsigned int *D=(unsigned int*)malloc(w*h*sizeof(unsigned int));
    for(i=0;i<w*h;i++)
        D[v1[i]]=C1[i];
    Salvare_memExt(imag_decript,D,"header.bin",w,h);
    free(C);free(R);free(v);free(v1);free(C1);free(D);
}
void chi_patrat (char *cale_imag)
{
    int w,h,i,j;
    Dimensiuni_img(cale_imag,&w,&h);//printf("\n%d %d\n",w,h);
    FILE *f=fopen(cale_imag,"rb");
    if(f==NULL)
    {
        printf("Eroare!");
        return;
    }
    float chi_r=0,chi_g=0,chi_b=0,f_bar;
    unsigned char *rgb=(unsigned char*)malloc(3);
    unsigned int *fr=(unsigned int *)malloc(256*sizeof(unsigned int)),*fg=(unsigned int *)malloc(256*sizeof(unsigned int)),*fb=(unsigned int *)malloc(256*sizeof(unsigned int));
    for(i=0;i<256;i++)
        {fr[i]=0;
        fg[i]=0;
        fb[i]=0;
        }
    f_bar=w*h/256;//printf("\n%.2f\n",f_bar);
    fseek(f,54,SEEK_SET);
    for(i=0;i<h;i++)
        for(j=0;j<w;j++)
            {
                fread(rgb,3,1,f);
                fr[rgb[2]]++;
                fg[rgb[1]]++;
                fb[rgb[0]]++;

            }
    for(i=0;i<256;i++)
        {
            chi_r=chi_r+((fr[i]-f_bar)*(fr[i]-f_bar))/f_bar;//printf("%2.f ",chi_r);
            chi_g+=(fg[i]-f_bar)*(fg[i]-f_bar)/f_bar;
            chi_b+=(fb[i]-f_bar)*(fb[i]-f_bar)/f_bar;
        }
    printf("\nRED: %.2f\nGREEN: %.2f\nBLUE: %.2f\n",chi_r,chi_g,chi_b);
    free(rgb);free(fr);free(fg);free(fb);
    fclose(f);
}


typedef struct
{
    int linie,coloana;
    float val_corelatie;
    unsigned char red,green,blue;

}Corelatii;
int max (int a,int b)
{
    if(a>b) return a;
    return b;
}
int min(int a,int b)
{
    if(a<b)
        return a;
    return b;
}
void Copy_Image(char *imagine,char *copie)
{
    FILE *f=fopen(imagine,"rb");
    if(f==NULL)
    {
        printf("Eroare!CI");
        return;
    }
    FILE *g=fopen(copie,"wb");
    char c;
    while(fread(&c,1,1,f)==1)
        fwrite(&c,1,1,g);
    fclose(f);fclose(g);
}
void grayscale_image(char* nume_fisier_sursa,char* nume_fisier_destinatie)
{
   FILE *fin, *fout;
   unsigned int dim_img, latime_img, inaltime_img;
   unsigned char pRGB[3], header[54], aux;

   //printf("nume_fisier_sursa = %s \n",nume_fisier_sursa);

   fin = fopen(nume_fisier_sursa, "rb");
   if(fin == NULL)
   	{
   		printf("nu am gasit imaginea sursa din care citesc");
   		return;
   	}

   fout = fopen(nume_fisier_destinatie, "wb+");

   fseek(fin, 2, SEEK_SET);
   fread(&dim_img, sizeof(unsigned int), 1, fin);
   //printf("Dimensiunea imaginii in octeti: %u\n", dim_img);

   fseek(fin, 18, SEEK_SET);
   fread(&latime_img, sizeof(unsigned int), 1, fin);
   fread(&inaltime_img, sizeof(unsigned int), 1, fin);
   //printf("Dimensiunea imaginii in pixeli (latime x inaltime): %u x %u\n",latime_img, inaltime_img);

   //copiaza octet cu octet imaginea initiala in cea noua
	fseek(fin,0,SEEK_SET);
	unsigned char c;
	while(fread(&c,1,1,fin)==1)
	{
		fwrite(&c,1,1,fout);
		fflush(fout);
	}
	fclose(fin);

	//calculam padding-ul pentru o linie
	int padding;
    if(latime_img % 4 != 0)
        padding = 4 - (3 * latime_img) % 4;
    else
        padding = 0;

    //printf("padding = %d \n",padding);

	fseek(fout, 54, SEEK_SET);
	int i,j;
	for(i = 0; i < inaltime_img; i++)
	{
		for(j = 0; j < latime_img; j++)
		{
			//citesc culorile pixelului
			fread(pRGB, 3, 1, fout);
			//fac conversia in pixel gri
			aux = 0.299*pRGB[2] + 0.587*pRGB[1] + 0.114*pRGB[0];
			pRGB[0] = pRGB[1] = pRGB[2] = aux;
        	fseek(fout, -3, SEEK_CUR);
        	fwrite(pRGB, 3, 1, fout);
        	fflush(fout);
		}
		fseek(fout,padding,SEEK_CUR);
	}
	fclose(fout);
}
unsigned char ** Matrice(char *imagine)
{
    FILE *f=fopen(imagine,"rb");
    if(f==NULL)
    {
        printf("Eroare!");
        return;
    }
    int w,h,i,j,x,padd=0;
    Dimensiuni_img(imagine,&w,&h);//printf("%d %d\n",w,h);
    if(w%4!=0)
        padd=(4-(3*w)%4)%4;
    unsigned char **L=(unsigned char **)malloc(h*sizeof(unsigned char*));
    for(i=0;i<h;i++)
        L[i]=(unsigned char*)malloc(w*sizeof(unsigned char));
    fseek(f,54,SEEK_SET);
    for(i=0;i<h;i++)
        {for(j=0;j<w;j++)
            {
                fread(&x,1,3,f);
                L[i][j]=x;
            }
        fseek(f,padd,SEEK_CUR);
        }
    fclose(f);
    return L;
}
float Corelatia(char *imagine,char *sablon,int l,int c,int ws,int hs,int wi,int hi,unsigned char**I,unsigned char**S)
{

    int i,j,n;
    n=ws*hs;//printf("%d\n",n);
    float corr,f_bar=0,s_bar=0,sigma_f=0,sigma_s=0;
    for(i=0;i<hs;i++)
        for(j=0;j<ws;j++)
        {
            f_bar+=I[l+i][c+j];
            s_bar+=S[i][j];
        }
    f_bar/=n;s_bar/=n;//printf("%.2f %.2f\n",f_bar,s_bar);
    for(i=0;i<hs;i++)
        for(j=0;j<ws;j++)
        {
            sigma_f+=(I[l+i][c+j]-f_bar)*(I[l+i][c+j]-f_bar);
            sigma_s+=(S[i][j]-s_bar)*(S[i][j]-s_bar);
        }
    sigma_f=sqrt(sigma_f/(n+1));
    sigma_s=sqrt(sigma_s/(n+1));//printf("%.2f %.2f\n",sigma_f,sigma_s);
    for(i=0;i<hs;i++)
        for(j=0;j<ws;j++)
            corr+=(I[l+i][c+j]-f_bar)*(S[i][j]-s_bar)/(sigma_f*sigma_s);
    corr/=n;//printf("%.2f\n",corr);
    return corr;
}
void Colorare_imag(char *imag,int l,int c,int ws,int hs,int wi,int hi,unsigned char *color)//parametrii l si c definesc fereastra fI=fI(l,c)
{
    FILE *f=fopen(imag,"rb+");
    if(f==NULL)
    {
        printf("Eroare!colorare");
        return;
    }
    int padd1=0,padd2=0,i,j;
    unsigned char *rgb=(unsigned char *)malloc(3);
    if(ws%4!=0)
        padd2=(4-(3*ws)%4)%4;
    if(wi%4!=0)
        padd1=(4-(3*wi)%4)%4;
    fseek(f,54+3*wi*l+padd1*l,SEEK_CUR);
    fseek(f,3*c,SEEK_CUR);
    for(i=0;i<ws;i++)
    {
        fread(rgb,1,3,f);
        rgb[0]=color[0];
        rgb[1]=color[1];
        rgb[2]=color[2];
        fseek(f,-3,SEEK_CUR);
        fwrite(rgb,1,3,f);
        fflush(f);
    }
    fseek(f,54+(l+hs)*(3*wi+padd1),SEEK_SET);
    fseek(f,3*c,SEEK_CUR);
    for(i=0;i<ws;i++)
    {

        fread(rgb,1,3,f);
        rgb[0]=color[0];
        rgb[1]=color[1];
        rgb[2]=color[2];
        fseek(f,-3,SEEK_CUR);
        fwrite(rgb,1,3,f);
        fflush(f);
    }
    fseek(f,54+3*wi*(l+1)+padd1*(l+1),SEEK_SET);
    for(i=1;i<hs;i++)
    {
        fseek(f,3*c,SEEK_CUR);
        fread(rgb,1,3,f);
        rgb[0]=color[0];
        rgb[1]=color[1];
        rgb[2]=color[2];
        fseek(f,-3,SEEK_CUR);
        fwrite(rgb,1,3,f);
        fflush(f);
        fseek(f,3*(wi-c-1)+padd1,SEEK_CUR);
    }
    fseek(f,54+3*wi*(l+1)+padd1*(l+1),SEEK_SET);
    for(i=1;i<hs;i++)
    {
        fseek(f,3*(c+ws-1),SEEK_CUR);
        fread(rgb,1,3,f);
        rgb[0]=color[0];
        rgb[1]=color[1];
        rgb[2]=color[2];
        fseek(f,-3,SEEK_CUR);
        fwrite(rgb,1,3,f);
        fflush(f);
        fseek(f,3*(wi-c-ws)+padd1,SEEK_CUR);
    }
    fclose(f);free(rgb);
}
Corelatii* template_matching(char *imagine,char *sablon,float prag,int *n)
{
    char *imagine_gs=(char *)malloc(20),*sablon_gs=(char *)malloc(20);
    imagine_gs="imagine_gs.bmp";
    sablon_gs="sablon_gs.bmp";
    grayscale_image(imagine,imagine_gs);
    grayscale_image(sablon,sablon_gs);
    int wi,hi,ws,hs,i,j,padd=0;(*n)=0;
    float corr;
    Dimensiuni_img(imagine,&wi,&hi);
    Dimensiuni_img(sablon,&ws,&hs);
    unsigned char **I=Matrice(imagine_gs),**S=Matrice(sablon_gs);
    FILE *f=fopen(imagine_gs,"rb+");
    if(f==NULL)
    {
        printf("Eroare!tm1");
        return;
    }
    FILE *g=fopen(sablon_gs,"rb");
    if(g==NULL)
    {
        printf("Eroare!tm2");
        return;
    }
    Corelatii *v=(Corelatii*)malloc(wi*hi*sizeof(Corelatii));
    if(ws%4!=0)
        padd=(4-(3*ws)%4)%4;
    int k=1;
    //printf("%d %d\n%d %d",wi,hi,ws,hs);
    for(i=0;i<hi-hs;i++)
        for(j=0;j<wi-ws;j++)
            {
                corr=Corelatia(imagine_gs,sablon_gs,i,j,ws,hs,wi,hi,I,S);//printf("%.2f %d\n",corr,k);k++;
                if(corr>prag)
                {
                    v[*n].linie=i;//printf("%.2f %d\n",corr,k);k++;
                    v[*n].coloana=j;
                    v[*n].val_corelatie=corr;
                    //printf("\n%.2f %d %d",corr,v[*n].linie,v[*n].coloana);
                    //Colorare_imag(imagine,i,j,ws,hs);
                    (*n)++;
                }
            }
    for(i=0;i<hi;i++)
        free(I[i]);
    free(I);
    for(i=0;i<hs;i++)
        free(S[i]);
    free(S);
    fclose(f);fclose(g);free(imagine_gs);free(sablon_gs);
    return v;
}
int compDescrescator(const void*a,const void *b)
{
    if(((Corelatii *)a)->val_corelatie<((Corelatii *)b)->val_corelatie)
        return 1;
    if(((Corelatii *)a)->val_corelatie>((Corelatii *)b)->val_corelatie)
        return -1;
    return 0;
}
Corelatii * Sortare_detectii(Corelatii *D,int m)
{
    qsort(D,m,sizeof(Corelatii),compDescrescator);
    return D;
}
float Suprapunere(Corelatii x,Corelatii y,int ws,int hs)
{
    float arie_x,arie_y,arie_int,s=0;
    arie_x=arie_y=ws*hs;
    int sus,jos,st,dr;
    jos=max(x.linie,y.linie);
    sus=min(x.linie+hs,y.linie+hs);
    st=max(x.coloana,y.coloana);
    dr=min(x.coloana+ws,y.coloana+ws);
    if(st<dr&&jos<sus)
        {arie_int=(dr-st)*(sus-jos);
         s=(arie_x+arie_y-arie_int)/arie_int;
        }
    return s;
}
Corelatii *Non_Maxime(Corelatii *D,int *m,int ws,int hs)
{
    D=Sortare_detectii(D,*m);
    float s=0;int k,i,j;
    //int nr=1;
    for(i=0;i<(*m)-1;i++)
        for(j=0;j<*m;j++)
            {
                s=Suprapunere(D[i],D[j],ws,hs);//printf("%d. %.2f\n",nr,s);nr++;
                if(s>0.20)
                {
                    for(k=j;k<(*m)-1;k++)
                        D[k]=D[k+1];
                    (*m)--;
                }
            }
    return D;
}

int main()
{
    //Modulul de criptare-decriptare

    char *imag_init=(char*)malloc(20);
    char *imag_cript=(char*)malloc(20);
    char *imag_decript=(char*)malloc(20);
    char *sec_k=(char*)malloc(20);
    printf("CRIPTARE\n\n");
    printf("Calea imaginii initiale: ");scanf("%s",imag_init);
    printf("\nCalea imaginii criptate: ");scanf("%s",imag_cript);
    printf("\nCalea fisierului ce contine cheia secreta: ");scanf("%s",sec_k);
    Criptare(imag_init,imag_cript,sec_k);
    printf("\n");
    printf("DECRIPTARE\n\n");
    printf("Calea imaginii criptate: ");scanf("%s",imag_cript);
    printf("\nCalea imaginii decriptate: ");scanf("%s",imag_decript);
    printf("\nCalea fisierului ce contine cheia secreta: ");scanf("%s",sec_k);
    Decriptare(imag_cript,imag_decript,sec_k);
    printf("\n");
    printf("CHI PATRAT\n\n");
    printf("Calea imaginii initiale: ");scanf("%s",imag_init);
    chi_patrat(imag_init);
    printf("\nCalea imaginii criptate: ");scanf("%s",imag_cript);
    chi_patrat(imag_cript);
    free(imag_cript);free(imag_init);free(sec_k);free(imag_decript);
    printf("\n\n");

    //Modulul de recunoastere de patternuri intr-o imagine
    printf("RECUNOASTERE DE PATTERNURI\n\n");
    char *imagine,*sablon;
    imagine=(char *)malloc(20);sablon=(char*)malloc(20);
    printf("Imaginea: ");scanf("%s",imagine);
    char *copie=(char *)malloc(20);copie="copie_imagine.bmp";
    Copy_Image(imagine,copie);
    int i,j,n,wi,hi,ws,hs;
    Dimensiuni_img(imagine,&wi,&hi);
    unsigned char *color=(unsigned char *)malloc(3);
    Corelatii *v;
    Corelatii *D=(Corelatii *)malloc(10*wi*hi*sizeof(Corelatii));//Am alocat 10*wi*hi*sizeof(Corelatii) deoarece este dimensiunea MAXIMA pe care o poate avea D (cazul in care
    int m=0;
    printf("\nCele 10 sabloane: ");                            //se poate pleca de la fiecare pixel al imaginii mari pentru a gasi o detectie, pentru fiecare sablon din cele 10).
    for(j=0;j<10;j++)
    {
        printf("\nSablonul %d: ",j);scanf("%s",sablon);
        Dimensiuni_img(sablon,&ws,&hs);
        switch(sablon[5])
        {
            case '0':color[0]=color[1]=0;color[2]=255;
                    v=template_matching(copie,sablon,0.50,&n);
                    break;
            case '1':color[2]=color[1]=255;color[0]=0;
                    v=template_matching(copie,sablon,0.50,&n);
                    break;
            case '2':color[0]=color[2]=0;color[1]=255;
                    v=template_matching(copie,sablon,0.50,&n);
                    break;
            case '3':color[0]=color[1]=255;color[2]=0;
                    v=template_matching(copie,sablon,0.50,&n);
                    break;
            case '4':color[0]=color[2]=255;color[1]=0;
                    v=template_matching(copie,sablon,0.50,&n);
                    break;
            case '5':color[2]=color[1]=0;color[0]=255;
                    v=template_matching(copie,sablon,0.50,&n);
                    break;
            case '6':color[0]=color[1]=color[2]=192;
                    v=template_matching(copie,sablon,0.50,&n);
                    break;
            case '7':color[0]=0;color[1]=140;color[2]=255;
                    v=template_matching(copie,sablon,0.50,&n);
                    break;
            case '8':color[0]=color[2]=128;color[1]=0;
                    v=template_matching(copie,sablon,0.50,&n);
                    break;
            case '9':color[0]=color[1]=0;color[2]=128;
                    v=template_matching(copie,sablon,0.50,&n);
                    break;
        }
        for(i=0;i<n;i++)
           {
           D[m].linie=v[i].linie;
           D[m].coloana=v[i].coloana;
           D[m].val_corelatie=v[i].val_corelatie;
           D[m].red=color[2];D[m].green=color[1];D[m].blue=color[0];
           m++;
           }
        free(v);
    }
    D=Non_Maxime(D,&m,ws,hs);
    for(i=0;i<m;i++)
    {
        color[0]=D[i].blue;
        color[1]=D[i].green;
        color[2]=D[i].red;
        Colorare_imag(imagine,D[i].linie,D[i].coloana,ws,hs,wi,hi,color);
    }
    //printf("\nm= %d",m);
    free(D);free(imagine);free(sablon);free(copie);
    return 0;
}
