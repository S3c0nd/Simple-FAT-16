#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>

FILE * fp;

const int SIZE=1048576;

unsigned char* head; 
unsigned char* FAT1;
unsigned char* FAT2;
unsigned char* FDT;
unsigned char* DATA;
int FATsize;
int FATCounter=0;
int FDTCounter=0;
int DATACounter=0;
int PresentDirClu=0;

struct tm *D_T;  
time_t t;    


typedef struct RootSubject
{
	unsigned char name[8];
	unsigned char extname[3];
	unsigned char prop;
	unsigned char blank[10];
	unsigned char time[2];
	unsigned char date[2];
	unsigned char firstclu[2];
	unsigned char flength[4];
}RS;

typedef struct BPB
{
	unsigned char jmpBOOT[3];
	unsigned char OEMname[8];
	unsigned char BytesPerSec[2];
	unsigned char SecPerCl;
	unsigned char RsvdSecCnt[2];
	unsigned char FATNum;
	unsigned char RootEntCnt[2];
	unsigned char TotSec[2];
	unsigned char Media;
	unsigned char FATSz[2];
	unsigned char SecPerTrk[2];
	unsigned char Headnum[2];
	unsigned char HiddSec[4];
	unsigned char TotSec2[4];
	unsigned char DrvNum;
	unsigned char Reserved;
	unsigned char BootSig;
	unsigned char VolD[4];
	unsigned char VolLab[11];
	unsigned char FileSysType[8];	
	unsigned char boot[448];
	unsigned char end[2];
} BPB;

BPB* bpb;

int synchFAT2(int fatsize)
{
	int j;
	for (j=0;j<fatsize;j++)
	{
		FAT2[j]=FAT1[j];
	}
	return 0;
}

int format()
{
	memset(head,0,SIZE);
	bpb=(BPB *)head;
	int i;
	bpb->jmpBOOT[0]=0xEb;
	bpb->jmpBOOT[1]=0x3c;
	bpb->jmpBOOT[2]=0x00;
	bpb->OEMname[0]=0x66;
	bpb->OEMname[1]=0x74;
	bpb->OEMname[2]=0x2e;
	bpb->OEMname[3]=0x63;	
	bpb->OEMname[4]=0x6f;
	for (i=5;i<=7;i++)
	{
		bpb->OEMname[i]=0x00;
	}
	bpb->BytesPerSec[0]=0x00;
	bpb->BytesPerSec[1]=0x02;
	bpb->SecPerCl=0x01;
	bpb->RsvdSecCnt[0]=0x20;
	bpb->RsvdSecCnt[1]=0x00;
	bpb->FATNum=0x02;
	bpb->RootEntCnt[0]=0x00;
	bpb->RootEntCnt[1]=0x00;
	bpb->TotSec[0]=0x00;
	bpb->TotSec[1]=0x08;
	bpb->Media=0xff;
	bpb->FATSz[0]=0x04;
	bpb->FATSz[1]=0x00;
	bpb->SecPerTrk[0]=0xff;
	bpb->SecPerTrk[1]=0xff;
	for (i=0;i<448;i++)
	{
		bpb->boot[i]=0x00;
	}
	bpb->Headnum[0]=0x00;
	bpb->Headnum[1]=0x00;
	for (i=0;i<=3;i++)
	{
		bpb->HiddSec[i]=0x00;
		bpb->TotSec2[i]=0x00;
		bpb->VolD[i]=0x00;
	}
	bpb->DrvNum=0x00;
	bpb->Reserved=0x00;
	bpb->BootSig=0x00;
	bpb->VolLab[0]=0x66;
	bpb->VolLab[1]=0x73;
	bpb->VolLab[2]=0x79;
	bpb->VolLab[3]=0x73;
	for (i=4;i<10;i++)
	{
		bpb->VolLab[i]=0x00;
	}	
	bpb->VolLab[10]='\0';
	bpb->FileSysType[0]='F';
	bpb->FileSysType[1]='A';
	bpb->FileSysType[2]='T';
	bpb->FileSysType[3]='1';
	bpb->FileSysType[4]='6';
	bpb->FileSysType[5]='\0';
	bpb->FileSysType[6]=0x00;
	bpb->FileSysType[7]=0x00;
	bpb->end[0]=0x55;
	bpb->end[1]=0xaa;
	FATsize=(bpb->FATSz[0]+bpb->FATSz[1]*256)*512;
	FAT1=head+512;
	FAT2=head+512+FATsize;
	FAT1[0]=0xff;
	FAT1[1]=0xff;
	FAT1[2]=0xff;
	FAT1[3]=0xff;
	FATCounter=3;	
	synchFAT2(FATsize);
	return 0;
}


int start()
{
	head=(unsigned char *)malloc(SIZE);
	memset(head,0x00,SIZE);
	if(fp=fopen("filesystem.disk","r"))
	{
		fread(head,SIZE,1,fp);
		bpb=(BPB *)head;
		FATsize=(bpb->FATSz[0]+bpb->FATSz[1]*256)*512;
		FAT1=head+512;
		FAT2=FAT1+FATsize;
		FDT=FAT2+FATsize;
		DATA=FDT+(bpb->RootEntCnt[0]+bpb->RootEntCnt[1]*256)*32;
		fclose(fp);
	}else
	{
		printf("the file does not exist yet, now create it!\n");
		fp=fopen("filesystem.disk","w+");
		format();
		FATsize=(bpb->FATSz[0]+bpb->FATSz[1]*256)*512;
		FAT1=head+512;
		FAT2=FAT1+FATsize;
		FDT=FAT2+FATsize;
		DATA=FDT+(bpb->RootEntCnt[0]+bpb->RootEntCnt[1]*256)*32;
		fwrite(head,SIZE,1,fp);
		fclose(fp);
	}
	PresentDirClu=0;
	return 0;
}

int mkdir(char *dirname,int SuperDirClu) //此处未加越界判定，即目录项数必须小于32，否则文件系统崩溃 
{
	unsigned char * SuperDir = (DATA+SuperDirClu*512);
	int DirCounter=0;
	while ((*(SuperDir+DirCounter*32))!=0x00)
	{
		DirCounter++;
		if(DirCounter>32)
			break;
	}
	RS* tempSub;
	tempSub=(RS*)(SuperDir+DirCounter);
//	memset(tempSub,0,sizeof(RS));
	strcpy((char*)tempSub->name,dirname);
	tempSub->prop=(1<<4);
	FATCounter=2;
	while (!((FAT1[FATCounter*2]==0x00)&&(FAT1[FATCounter*2+1]==0x00)))
		FATCounter++;
	FAT1[FATCounter*2]=0xff;
	FAT1[FATCounter*2+1]=0xff;
	tempSub->firstclu[1]=FATCounter/256;
	tempSub->firstclu[0]=FATCounter%256;
	t=time(NULL);
	D_T=localtime(&t);	
	tempSub->time[0]=(unsigned char)D_T->tm_hour;    //此处仅记录了时分和月日，需改进 
	tempSub->time[1]=(unsigned char)D_T->tm_min;
	tempSub->date[0]=(unsigned char)D_T->tm_mon;
	tempSub->date[1]=(unsigned char)D_T->tm_mday;
	DATACounter=FATCounter;
	tempSub=(RS*)(DATA+DATACounter*512);
	strcpy((char *)(tempSub->name),"..");
//	memset(tempSub,0,sizeof(RS));
	tempSub->firstclu[0]=SuperDirClu%256;
	tempSub->firstclu[1]=SuperDirClu/256;
	tempSub->prop=0x10;			
	return 0;
}

int cd(char * dirname,int SuperDirClu)
{
	
	unsigned char * SuperDir = (DATA+SuperDirClu*512);
	int DirCounter=0;
	RS* tempSub;
	for (DirCounter=0;DirCounter<32;DirCounter++)
	{
		tempSub=(RS*)(SuperDir+DirCounter);
		if (!strcmp((char *)tempSub->name,dirname))
			break;
	}
	if ((strcmp((char*)tempSub->name,dirname))||(tempSub->prop!=0x10))
	{
		printf("Director does not exist!\n");
	}else
	{
		PresentDirClu=tempSub->firstclu[0]+tempSub->firstclu[1]*256;
	}
	
	return 0;
}

int writef(char* filename,unsigned char * data,int FileSize,int SuperDirClu)   //如有问题可改为仅支持小文件 
{
	int write_i;
	FATCounter = 2;
	unsigned char * lastclu;
	unsigned char * SuperDir = (DATA+SuperDirClu*512);
	int DirCounter=0;
	while ((*(SuperDir+DirCounter*32))!=0x00)
	{
		DirCounter++;
	}
	RS* tempSub;
	tempSub=(RS*)(SuperDir+DirCounter);
	memset(tempSub,0,sizeof(RS));
	while ((FAT1[FATCounter*2]!=0x00)&&(FAT1[FATCounter*2+1]!=0x00))
		FATCounter++;
	tempSub->firstclu[1]=FATCounter/256;
	tempSub->firstclu[0]=FATCounter%256;
	FATCounter = 2;
	strcpy((char*)tempSub->name,filename);
	t=time(NULL);
	D_T=localtime(&t);	
	tempSub->time[0]=(unsigned char)D_T->tm_hour;    //此处仅记录了时分和月日，需改进 
	tempSub->time[1]=(unsigned char)D_T->tm_min;
	tempSub->date[0]=(unsigned char)D_T->tm_mon;
	tempSub->date[1]=(unsigned char)D_T->tm_mday;
	int tempsize=FileSize;	
	for (write_i=0;write_i<4;write_i++)
	{
		tempSub->flength[write_i]=tempsize%256;
		tempsize/=256;
	}			
//	while (FileSize>0)
//	{
		FATCounter=2;
		while ((FAT1[FATCounter*2]!=0x00)&&(FAT1[FATCounter*2+1]!=0x00))
			FATCounter++;
		DATACounter=FATCounter;	
		strcpy((char *)(DATA+DATACounter*512),(char *)data);
		FAT1[FATCounter*2]=0xff;
		FAT1[FATCounter*2+1]=0xff;
		/*
		lastclu=FAT1+FATCounter*2;
		DATACounter=FATCounter;
		for (write_i=0;write_i<512;write_i++)
		{
			DATA[DATACounter*512+write_i]=data[write_i];
		}
		FileSize-=512;
		if (FileSize<0)
			{
				*lastclu=FATCounter/256;
				*(lastclu+1)=FATCounter%256;
				FAT1[FATCounter*2]=0xff;
				FAT1[FATCounter*2+1]=0xff;
			}else
			{
				*lastclu=FATCounter/256;
				*(lastclu+1)=FATCounter%256;				
			}
			*/
//	}
	synchFAT2(FATsize);
	return 0;
}

int openf(char * filename,int SuperDirClu)
{
	unsigned char * filedata;
	unsigned char * SuperDir = (DATA+SuperDirClu*512);
	int open_i=0;
	int DirCounter=0;
	RS* tempSub;
	for (DirCounter=0;DirCounter<32;DirCounter++)
	{
		tempSub=(RS*)(SuperDir+DirCounter);
		if (!strcmp((char *)tempSub->name,filename))
			break;
	}
	if ((strcmp((char*)tempSub->name,filename))||(tempSub->prop==0x10))
	{
		printf("File does not exist!\n");
	}else
	{
		int fileclu=tempSub->firstclu[0]+tempSub->firstclu[1]*256;
		int filesize=0;
		for (open_i=0;open_i<4;open_i++)
		{
			filesize*=256;
			filesize+=tempSub->flength[open_i];
		}
/*		while (filesize>0)
		{*/
			filedata=(DATA+fileclu*512);
			printf("%s",(char *)filedata);
/*			if (filesize>512)
			{
				for (open_i=0;open_i<512;open_i++)
				{
					printf("%c",filedata[open_i]);
				}
				filesize-=512;
			}else
			{
				for (open_i=0;open_i<512;open_i++)
				{
					printf("%c",filedata[open_i]);
				}
				filesize-=512;
			}
			
			fileclu=FAT1[fileclu*2]+FAT1[fileclu*2+1]*256;
		}*/
	}
	return 0;
}

int del(char * filename,int SuperDirClu)
{
	unsigned char * filedata;
	unsigned char * SuperDir = (DATA+SuperDirClu*512);
	int del_i=0;
	int DirCounter=0;
	RS* tempSub;
	for (DirCounter=0;DirCounter<32;DirCounter++)
	{
		tempSub=(RS*)(SuperDir+DirCounter);
		if (!strcmp((char *)tempSub->name,filename))
			break;
	}
	if ((strcmp((char*)tempSub->name,filename))||(tempSub->prop==0x10))
	{
		printf("Director does not exist!\n");
	}else
	{
		int temp;
		((unsigned char *)tempSub)[0]=0xe5;
		int fileclu=tempSub->firstclu[0]+tempSub->firstclu[1]*256;
		do
		{
			temp=fileclu;
			FAT1[fileclu*2]=0;
			FAT1[fileclu*2+1]=0;
			fileclu=FAT1[temp*2]+FAT1[temp*2+1]*256;
		}while(fileclu!=0xffff);
	}
	return 0;
}

int dir()
{
	RS* PresentAdd=(RS*)(DATA+PresentDirClu*512);
	int dir_i;
	for(dir_i=0;dir_i<32;dir_i++)
	{
		printf("%s",(PresentAdd+dir_i)->name);
	}
	
	return 0;
}

int main()
{
		start();
		while (1)
		{
			char order[20];
			char cnt[20];
			char name[20];
			printf("root@filesystem  : /");
			scanf("%s",order);
			if(!strcmp(order,"mkdir"))
			{
				char dirname[20];
				scanf("%s",dirname);
				mkdir(dirname,PresentDirClu);
			}
			if(!strcmp(order,"dir"))
			{
				dir();
			}
			if (!strcmp(order,"cd"))
			{
				char dirname[20];
				scanf("%s",dirname);
				cd(dirname,PresentDirClu);
			}
			if (!strcmp(order,"open"))
			{
				char fname[20];
				scanf("%s",fname);
				openf(fname,PresentDirClu);				
			}
			if (!strcmp(order,"write"))
			{
				char fname[20];
				char fcnt[100];
				scanf("%s%s",fcnt,fname);
				writef(fname,(unsigned char *)fcnt,strlen(fcnt)+1,PresentDirClu);				
			}
			if (!strcmp(order,"exit"))
			{
				fopen("filesystem.disk","w");
			int d=fwrite(head,SIZE,1,fp);
			printf("%d",d);
			fclose(fp);
			break;
			}
		}

	return 0;
}
