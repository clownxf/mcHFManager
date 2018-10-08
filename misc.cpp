#include <vcl.h>
#pragma hdrstop

#include <stdio.h>
#include "misc.h"

void DebugPrintHexArray(UCHAR *pArray,UINT aSize,char *chMessage)
{
	UINT i = 0,j = 0, c = 0;
	char *buf;

	buf = (char *)malloc(aSize * 10);

	if(chMessage)
	{
		strcpy(buf,"NSS: ");
		strcat(buf,chMessage);
		OutputDebugString(buf);
		memset(buf,0,aSize);
	}

	j += sprintf( buf+j ,"NSS: ");

	for (i=0;i<aSize;i++)
	{

		j += sprintf( buf+j ,"%02X ", *pArray );
		pArray++;
		if (c++==15)
		{
			j += sprintf( buf+j ,"\n" );
			j += sprintf( buf+j ,"NSS: ");
			c = 0;
		}
	}
	OutputDebugString(buf);
	free(buf);
}

void DebugPrintInt(char *text,UINT val)
{
	char buf[260];

	sprintf(buf ,"NSS: %s = %08X ", text, val );
	OutputDebugString(buf);
}

char *ReturnHexArray(UCHAR *pArray,UINT aSize)
{
	UINT i = 0, j = 0;
	char *buf;

	buf = (char *)malloc(aSize*2 + 10);

	for (i = 0; i < aSize;i++)
	{
		j += sprintf(buf+j ,"%02X", *pArray);
		pArray++;
	}

	return buf;
}

// Moded, not original !!!!!
bool AnyFileOpen(char *nPath, ULONG *pBuffer, ULONG *bReaded)
{
    FILE *stream;
    UCHAR *fBuffer;
	ULONG fSize = 0,alocSize;
	ULONG bCount = 0;

	// open
	if((stream = fopen( nPath, "rb" )) != NULL )
	{
	  // Check file size
	  fseek (stream, 0L, SEEK_END);
	  fSize = ftell(stream);
	  fseek (stream, 0l, SEEK_SET);
	  if (fSize == 0)
		return false;

	  //DebugPrintInt("fSize",fSize);

	  // mcHF correction
	  if(fSize%512)
		alocSize = (fSize/512 + 1) * 512;
	  else
		alocSize = fSize;

	  //DebugPrintInt("alocSize",alocSize);

	  // Allocate memory
	  fBuffer =(UCHAR *)malloc(alocSize);

	  // Clear
	  memset(fBuffer,0,alocSize);

	  // Read bytes
	  bCount = fread(fBuffer, sizeof (char), fSize, stream);
	  if (bCount == 0)
	    return false;

      fclose( stream );

      *bReaded = alocSize;	// bCount;, return allocated, not read size!
      *pBuffer =(ULONG) fBuffer;

    }
	else
		return false;

	return true;
}
