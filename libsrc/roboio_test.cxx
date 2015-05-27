/*
 * roboio_test.cxx
 * 
 * Copyright 2015 root <root@pviewfull>
 * 
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
 * MA 02110-1301, USA.
 * 
 * 
 */

#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>
#include <sys/io.h>
#include "dxio.h"


// Roboio lib test
// cpu id
// -lRBIO


// Change DWORD to Byte array
void ChangeByteOrder(unsigned long tmp, unsigned char *p);
// Print ISOinChip data
void DumpData(char *s, unsigned char array[], int begin, int end, int format);

// Print ISOinChip data
// *s           data title
// array[]      ISOinChip data array
// begin        start offset
// end          end offset
// format       format flag
void DumpData(char *s, unsigned char array[], int begin, int end, int format)
        {
         printf("\n %s = ", s);
         for(; begin <= end; begin++)
          {
           if(format)
           printf("%02x", array[begin]);
           else
           printf("%c", array[begin]);
          }
        }

// Change DWORD to byte array
void ChangeByteOrder(unsigned long tmp, unsigned char *p)
        {
         p[0] = tmp >> 0;
         p[1] = tmp >> 8;
         p[2] = tmp >> 16;
         p[3] = tmp >> 24;
        }

void isoInCchip() {
	// ISOinChip array
	unsigned char ISOinChip[32] = { 0 };
	unsigned long  mtbf_counter=0;
	unsigned long  mtbf_counter1=0;
	unsigned long  mtbf_counter2=0;
	unsigned long  mtbf_counter3=0;
	unsigned long  mtbf_counter4=0;
	int i = 0;
	
	for(; i < 8; i++)
	{
	// Read ISOinChip data in PCI north bridge
	outl(0x800000D0 + i * sizeof(unsigned long), 0xCF8);
	ChangeByteOrder(inl(0xCFC), ISOinChip + i * sizeof(unsigned long));
	}
  
	mtbf_counter1=inb(0x72);
	mtbf_counter2=inb(0x73);
	mtbf_counter3=inb(0x74);
	mtbf_counter4=inb(0x75);
	
	mtbf_counter=mtbf_counter1+(mtbf_counter2<<8)+(mtbf_counter3<<16)+(mtbf_counter4<<24);


	// Print CPU ID data at offset 00h-05h
	DumpData("CPU ID", ISOinChip, 0, 5, 1);

	// Print product name data at offset 06h-0Dh
	DumpData("Product Name", ISOinChip, 6, 13, 0);

	// Print PCB version at offset 0Eh-13h
	DumpData("PCB Version", ISOinChip, 14, 19, 0);

	// Print export week at offset 14h-17h
	DumpData("Export Week", ISOinChip, 20, 23, 0);

	// Print user ID at offset 18h-1Fh
	DumpData("USER ID", ISOinChip, 24, 31, 1);

	// Print mtbf counter
	printf("\n MTBF counter = %ld ", mtbf_counter);
	 printf("\n MTBF counter = %ld ", mtbf_counter2);
	 printf("\n MTBF counter = %ld ", mtbf_counter3);
	 printf("\n MTBF counter = %ld ", mtbf_counter4);

	printf("\n");
}

int main(int argc, char *argv[])
{
	io_Init();

 	switch (io_CpuID()) {
		case CPU_UNSUPPORTED:
			printf("CPU unsupported.\n");
			exit(1);
			break;
		case CPU_VORTEX86SX:
			printf("CPU is vortex86sx\n");
			break;
		case CPU_VORTEX86DX_1:
			printf("CPU is vortex86dx 1\n");
			break;
		case CPU_VORTEX86DX_2:
			printf("CPU is vortex86dx 2\n");
			break;
		case CPU_VORTEX86DX_3:
			printf("CPU is vortex86dx 3\n");
			break;
		case CPU_VORTEX86DX_UNKNOWN:
			printf("CPU is vortex86dx unknown\n");
			break;
		case CPU_VORTEX86MX:
			printf("CPU is vortex86mx\n");
			break;
		case CPU_VORTEX86MX_PLUS:
			printf("CPU is vortex86dx_3\n");
			break;
 	}
 	
 	isoInCchip();
 	
	exit(0);

}
