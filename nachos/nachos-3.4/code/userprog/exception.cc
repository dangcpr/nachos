﻿// exception.cc 
//	Entry point into the Nachos kernel from user programs.
//	There are two kinds of things that can cause control to
//	transfer back to here from user code:
//
//	syscall -- The user code explicitly requests to call a procedure
//	in the Nachos kernel.  Right now, the only function we support is
//	"Halt".
//
//	exceptions -- The user code does something that the CPU can't handle.
//	For instance, accessing memory that doesn't exist, arithmetic errors,
//	etc.  
//
//	Interrupts (which can also cause control to transfer from user
//	code into the Nachos kernel) are handled elsewhere.
//
// For now, this only handles the Halt() system call.
// Everything else core dumps.
//
// Copyright (c) 1992-1993 The Regents of the University of California.
// All rights reserved.  See copyright.h for copyright notice and limitation 
// of liability and disclaimer of warranty provisions.

#include "copyright.h"
#include "system.h"
#include "syscall.h"
#include "filehdr.h"
#define MaxFileLength 255
//----------------------------------------------------------------------
// ExceptionHandler
// 	Entry point into the Nachos kernel.  Called when a user program
//	is executing, and either does a syscall, or generates an addressing
//	or arithmetic exception.
//
// 	For system calls, the following is the calling convention:
//
// 	system call code -- r2
//		arg1 -- r4
//		arg2 -- r5
//		arg3 -- r6
//		arg4 -- r7
//
//	The result of the system call, if any, must be put back into r2. 
//
// And don't forget to increment the pc before returning. (Or else you'll
// loop making the same system call forever!
//
//	"which" is the kind of exception.  The list of possible exceptions 
//	are in machine.h.
//----------------------------------------------------------------------

// Input: - User space address (int)
// - Limit of buffer (int)
// Output:- Buffer (char*)
// Purpose: Copy buffer from User memory space to System memory space
char* User2System(int virtAddr, int limit)
{
	int i; //chi so index
	int oneChar;
	char* kernelBuf = NULL;
	kernelBuf = new char[limit + 1]; //can cho chuoi terminal
	if (kernelBuf == NULL)
		return kernelBuf;

	memset(kernelBuf, 0, limit + 1);

	for (i = 0; i < limit; i++)
	{
		machine->ReadMem(virtAddr + i, 1, &oneChar);
		kernelBuf[i] = (char)oneChar;
		if (oneChar == 0)
			break;
	}
	return kernelBuf;
}

// Input: - User space address (int)
// - Limit of buffer (int)
// - Buffer (char[])
// Output:- Number of bytes copied (int)
// Purpose: Copy buffer from System memory space to User memory space
int System2User(int virtAddr, int len, char* buffer)
{
	if (len < 0) return -1;
	if (len == 0)return len;
	int i = 0;
	int oneChar = 0;
	do {
		oneChar = (int)buffer[i];
		machine->WriteMem(virtAddr + i, 1, oneChar);
		i++;
	} while (i < len && oneChar != 0);
	return i;
}

// Inscrease program counter
// Pre-PC register assigned by PC register
// PC register assigned by Next-PC register
// Next-PC resgister assigned by 4-byte ahead register
void InscreasePC()
{
	machine->registers[PrevPCReg] = machine->registers[PCReg];
	machine->registers[PCReg] = machine->registers[NextPCReg];
	machine->registers[NextPCReg] += 4;
}

// Ham xu ly ngoai le runtime Exception va system call
void ExceptionHandler(ExceptionType which)
{
	int type = machine->ReadRegister(2);

	// Bien toan cuc cho lop SynchConsole
	// Bat dau
	switch (which) {
	case SyscallException:
		switch (type) {

		case SC_Halt:
		{
			// Input: Khong co
			// Output: Thong bao tat may
			// Chuc nang: Tat HDH
			printf("\nShutdown, initiated by user program.\n");
			interrupt->Halt();
		}
		break;
		case SC_CreateFile:
		{
			int virtAddr;
			char* filename;
			
			virtAddr = machine->ReadRegister(4); 

			filename = User2System(virtAddr, MaxFileLength + 1);
			if (strlen(filename) == 0)
			{
				printf("\n File name is not valid");
				machine->WriteRegister(2, -1); //Return -1 vao thanh ghi R2
				delete[] filename;
				break;
			}

			if (filename == NULL)  //Neu khong doc duoc
			{
				printf("\n Not enough memory in system");
				//DEBUG('a', "\n Not enough memory in system");
				machine->WriteRegister(2, -1); //Return -1 vao thanh ghi R2
				delete[] filename;
				break;
			}
			//DEBUG('a', "\n Finish reading filename.");

			if (!fileSystem->Create(filename, 0)) //Tao file bang ham Create cua fileSystem, tra ve ket qua
			{
				//Tao file that bai
				printf("\nError create file '%s'", filename);
				machine->WriteRegister(2, -1);
				delete[] filename;
				break;
			}

			//Tao file thanh cong
			printf("\nCreate file '%s' success", filename);
			machine->WriteRegister(2, 0);
			delete[] filename;
			break;
		}
		case SC_Open:
		{
			int bufAddr = machine->ReadRegister(4); // read string pointer from user
			int type = machine->ReadRegister(5);
			char *buf;


			if (fileSystem->index > 10)
			{
				machine->WriteRegister(2, -1);
				delete[] buf;
				break;
			}
			buf = User2System(bufAddr, MaxFileLength + 1);
			if (strcmp(buf, "stdin") == 0)
			{
				printf("Stdin mode\n");
				//fileSystem->index++;
				machine->WriteRegister(2, 0);
				delete[] buf;
				break;
			}
			if (strcmp(buf, "stdout") == 0)
			{
				printf("Stdout mode\n");
				//fileSystem->index++;
				machine->WriteRegister(2, 1);
				delete[] buf;
				break;
			}

			if ((fileSystem->openfile[fileSystem->index] = fileSystem->Open(buf, type)) != NULL)
			{

				printf("\nOpen file success '%s'\n", buf);
				machine->WriteRegister(2, fileSystem->index - 1);
			}
			else
			{
				printf("Can not open file '%s'", buf);
				machine->WriteRegister(2, -1);
			}
			delete[] buf;
			break;

		}
		case SC_Close:
		{
			int no = machine->ReadRegister(4);
			int i = fileSystem->index;

			if (i < no)
			{
				printf("Close file failed \n");
				machine->WriteRegister(2, -1);
				break;
			}

			fileSystem->openfile[no] == NULL;
			delete fileSystem->openfile[no];
			machine->WriteRegister(2, 0);
			printf("Close file success\n");
			break;
		}
		case SC_Read:
		{
			int virtAddr = machine->ReadRegister(4);
			int charcount = machine->ReadRegister(5);
			int openf_id = machine->ReadRegister(6);
			int i = fileSystem->index;
			//printf("%d", virtAddr);
			if (openf_id > i || openf_id < 0 || openf_id == 1) // sdout
			{
				printf("Try to open invalib file");
				machine->WriteRegister(2, -1);
				break;
			}

			if (fileSystem->openfile[openf_id] == NULL)
			{
				machine->WriteRegister(2, -1);
				//printf("NULL");
				//delete temp;
				break;
			}

			//int before = temp->seekPosition;
			//printf("%d", start);
			char *buf = User2System(virtAddr, charcount);
			//printf("%s", buf);
			if (openf_id == 0) // stdin
			{
				int sz = gSynchConsole->Read(buf, charcount);
				System2User(virtAddr, sz, buf);
				machine->WriteRegister(2, sz);

				delete[] buf;
				//delete temp;
				break;
			}
			int before = fileSystem->openfile[openf_id]->GetCurrentPos();
			if ((fileSystem->openfile[openf_id]->Read(buf, charcount)) > 0)
			{
				// Copy data from kernel to user space
				int after = fileSystem->openfile[openf_id]->GetCurrentPos();
				System2User(virtAddr, charcount, buf);
				machine->WriteRegister(2, after - before + 1);
			} else {
				machine->WriteRegister(2, -1);
			}
			delete[] buf;
			//delete temp;
			break;
			
		}
		case SC_Write:
		{
			int virtAddr = machine->ReadRegister(4);
			int charcount = machine->ReadRegister(5);
			int openf_id = machine->ReadRegister(6);
			int i = fileSystem->index;

			//printf("%d",fileSystem->openfile[openf_id]->type); 

			if (openf_id > i || openf_id < 0 || openf_id == 0) // stdin
			{
				machine->WriteRegister(2, -1);
				break;
			}
			
			if (fileSystem->openfile[openf_id] == NULL)
			{
				machine->WriteRegister(2, -1);
				//printf("NULL");
				//delete temp;
				break;
			}

			// read-only file	
			if (fileSystem->openfile[openf_id]->type == 1)
			{
				printf("Try to modify read-only file");
				machine->WriteRegister(2, -1);
				break;
			}

			char *buf = User2System(virtAddr, charcount);
			// print out to console
			if (openf_id == 1)
			{
				int i = 0;
				while (buf[i] != '\0' && buf[i] != '\n')
				{
					gSynchConsole->Write(buf + i, 1);
					i++;
				}
				buf[i] = '\n';
				gSynchConsole->Write(buf + i, 1); // write last character

				machine->WriteRegister(2, i - 1);
				delete[] buf;
				//delete temp;
				break;
			}


			// write into file
			int before = fileSystem->openfile[openf_id]->GetCurrentPos();
			//printf("%s", buf);
			if ((fileSystem->openfile[openf_id]->Write(buf, charcount)) != 0)
			{
				//printf("%s", buf);
				int after = fileSystem->openfile[openf_id]->GetCurrentPos();
				System2User(virtAddr, after - before, buf);
				machine->WriteRegister(2, after - before + 1);
				delete[] buf;
				//delete temp;
				break;
			}
		}
		case SC_Seek:
		{
			int pos = machine->ReadRegister(4);
			int openf_id = machine->ReadRegister(5);

			// seek into files: stdin, stdout, out of fileSystem->index
			if (openf_id < 1 || openf_id > fileSystem->index || fileSystem->openfile[openf_id] == NULL)
			{
				machine->WriteRegister(2, -1);
				break;
			}

			int len = fileSystem->openfile[openf_id]->Length();
			//printf("%d", len);
			if (pos > len)
			{
				machine->WriteRegister(2, -1);
				break;
			}

			if (pos == -1)
				pos = len;

			fileSystem->openfile[openf_id]->Seek(pos);
			machine->WriteRegister(2, pos);
			break;
		}
		case SC_Print:
		{
			int virtAddr = machine->ReadRegister(4);
			int i = 0;
			char *buf = new char[MaxFileLength];
			buf = User2System(virtAddr, MaxFileLength + 1);
			while (buf[i] != 0 && buf[i] != '\n')
			{
				gSynchConsole->Write(buf + i, 1);
				i++;
			}
			//buf[i] = '\n';
			gSynchConsole->Write(buf + i, 1);
			delete[] buf;
			break;
		}
		case SC_Scan:
		{
			char *buf = new char[MaxFileLength];
			if (buf == 0) // out of save space
			{
				delete[] buf;
				break;
			}

			int virtAddr = machine->ReadRegister(4);
			int length = machine->ReadRegister(5);

			int sz = gSynchConsole->Read(buf, length);
			System2User(virtAddr, sz, buf);
			delete[] buf;
			break;
		}
		case SC_PrintChar:
		{
			char ch;
			ch = (char) machine->ReadRegister(4);
			gSynchConsole->Write(&ch, 1);
			break;
		}
		}
		

	InscreasePC();
		break;

	case NoException:
		interrupt->Halt();
		return;

	case PageFaultException:
		printf("\nNo valid translation found.\n");
		ASSERT(FALSE);
		break;

	case ReadOnlyException:
		printf("\nWrite attempted to page marked \"read-only\".\n");
		ASSERT(FALSE);
		break;

	case BusErrorException:
		printf("\nTranslation resulted in an invalid physical address.\n");
		ASSERT(FALSE);
		break;

	case AddressErrorException:
		printf("\nUnaligned reference or one that was beyond the end of the address space.\n");
		ASSERT(FALSE);
		break;

	case OverflowException:
		printf("\nInteger overflow in add or sub.\n");
		ASSERT(FALSE);
		break;

	case IllegalInstrException:
		printf("\nUnimplemented or reserved instr\n");
		ASSERT(FALSE);
		break;

	case NumExceptionTypes:
		printf("\nNumExceptionTypes\n");
		ASSERT(FALSE);
		break;

	}
	return;
}
