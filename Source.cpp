#include <Windows.h>
#include <fstream>
#include <iostream>

using namespace std;

int rounded_down(int number, int to_round) {
	double n = (double)number;
	double tr = (double)to_round;
	n /= tr;
	return (int)floor(n) * tr;
}
int rounded(int number, int to_round) {
	double n = (double)number;
	double tr = (double)to_round;
	n /= tr;
	cout << "N: " << hex << n << endl;
	cout << "Tr: " << hex << tr << endl;
	return (int)floor(n + 0.5) * tr;
}

void AddNumOfSections(IMAGE_NT_HEADERS* NtHeader) {
	IMAGE_FILE_HEADER* fHeader = &NtHeader->FileHeader;
	WORD new_NumberOfSections = fHeader->NumberOfSections + 1;
	memcpy(&fHeader->NumberOfSections, &new_NumberOfSections, sizeof(new_NumberOfSections));
	cout << "Numero se secciones finales: " << fHeader->NumberOfSections << endl;
}

void printSectionHeader(IMAGE_SECTION_HEADER* SectionHeader) {
	cout << "--------------------------------------" << endl;
	cout << "Name: " << SectionHeader->Name << endl;
	cout << hex << "VirtualSize: " << SectionHeader->Misc.VirtualSize << endl;
	cout << hex << "VirtualAddress: " << SectionHeader->VirtualAddress << endl;
	cout << hex << "SizeOfRawData: " << SectionHeader->SizeOfRawData << endl;
	cout << hex << "PointerToRawData: " << SectionHeader->PointerToRawData << endl;
	cout << hex << "Characteristics: " << SectionHeader->Characteristics << endl;
	cout << "--------------------------------------" << endl;
	cout << endl;
}

void AddSectionToEnd(void* rawData, IMAGE_DOS_HEADER* DOSHeader, IMAGE_NT_HEADERS* NtHeader, int sizeFile) {
	int numOfSec = NtHeader->FileHeader.NumberOfSections;
	
	IMAGE_OPTIONAL_HEADER* OpHeader = &NtHeader->OptionalHeader;
	IMAGE_SECTION_HEADER* LastSectionHeader = PIMAGE_SECTION_HEADER(DWORD(rawData) + DOSHeader->e_lfanew + 248 + ((numOfSec-2) * 40));
	IMAGE_SECTION_HEADER* NewSectionHeader = PIMAGE_SECTION_HEADER(DWORD(rawData) + DOSHeader->e_lfanew + 248 + ((numOfSec-1) * 40));

	int RVA_newSec = rounded(LastSectionHeader->VirtualAddress + LastSectionHeader->Misc.VirtualSize, OpHeader->SectionAlignment);
	cout << "VirtualAddress: "<< hex << LastSectionHeader->VirtualAddress << endl;
	cout << "Misc.VirtualSize: " << hex << LastSectionHeader->Misc.VirtualSize << endl;
	cout << "SectionAlignment: " << hex << OpHeader->SectionAlignment << endl;
	BYTE name = (BYTE)'INVE';
	DWORD new_Characteristics = 0xE0000080;
	DWORD new_PointerToRawData = LastSectionHeader->PointerToRawData + LastSectionHeader->SizeOfRawData;
	cout << "Misc.VirtualSize: " << hex << LastSectionHeader->PointerToRawData << endl;
	cout << "SectionAlignment: " << hex << LastSectionHeader->SizeOfRawData << endl;
	memcpy(&NewSectionHeader->Name, &name, sizeof(BYTE));
	memcpy(&NewSectionHeader->Misc.VirtualSize, &sizeFile, sizeof(DWORD));
	memcpy(&NewSectionHeader->VirtualAddress, &RVA_newSec, sizeof(DWORD));
	memcpy(&NewSectionHeader->PointerToRawData, &new_PointerToRawData, sizeof(DWORD));
	memcpy(&NewSectionHeader->Characteristics, &new_Characteristics, sizeof(DWORD));
	ZeroMemory(&NewSectionHeader->SizeOfRawData, sizeof(DWORD));
	ZeroMemory(&NewSectionHeader->PointerToRelocations, sizeof(DWORD));
	ZeroMemory(&NewSectionHeader->PointerToLinenumbers, sizeof(DWORD));
	ZeroMemory(&NewSectionHeader->NumberOfRelocations, sizeof(WORD));
	ZeroMemory(&NewSectionHeader->PointerToLinenumbers, sizeof(WORD));
	printSectionHeader(NewSectionHeader);
}

void AddSection(void* rawData, IMAGE_DOS_HEADER* DOSHeader, IMAGE_NT_HEADERS* NtHeader, int sizeFile) {
	
	BOOL isFirst = TRUE;
	IMAGE_SECTION_HEADER* SectionHeader;
	char* prevSectionHeaderBuf = new char[sizeof(IMAGE_SECTION_HEADER)];
	char* actSectionHeaderBuf = new char[sizeof(IMAGE_SECTION_HEADER)];

	for (int count = 0; count < NtHeader->FileHeader.NumberOfSections; count++)
	{
		SectionHeader = PIMAGE_SECTION_HEADER(DWORD(rawData) + DOSHeader->e_lfanew + 248 + (count * 40));
		memcpy(actSectionHeaderBuf, prevSectionHeaderBuf, sizeof(IMAGE_SECTION_HEADER));
		memcpy(prevSectionHeaderBuf, SectionHeader, sizeof(IMAGE_SECTION_HEADER));

		if (isFirst) {
			//ZeroMemory(SectionHeader, sizeof(PIMAGE_SECTION_HEADER));
			BYTE name = (BYTE)'h1';
			DWORD new_Characteristics = 0xE0000080;
			memcpy(&SectionHeader->Name, &name, sizeof(BYTE));
			memcpy(&SectionHeader->Misc.VirtualSize, &sizeFile, sizeof(int));
			ZeroMemory(&SectionHeader->SizeOfRawData, sizeof(DWORD));
			ZeroMemory(&SectionHeader->PointerToRelocations, sizeof(DWORD));
			ZeroMemory(&SectionHeader->PointerToLinenumbers, sizeof(DWORD));
			ZeroMemory(&SectionHeader->NumberOfRelocations, sizeof(WORD));
			ZeroMemory(&SectionHeader->PointerToLinenumbers, sizeof(WORD));
			memcpy(&SectionHeader->Characteristics, &new_Characteristics, sizeof(DWORD));
			printSectionHeader(SectionHeader);
			isFirst = FALSE;
		}
		else{
			memcpy(SectionHeader, actSectionHeaderBuf, sizeof(IMAGE_SECTION_HEADER));
			DWORD new_VirtualAddress = SectionHeader->VirtualAddress + sizeFile;
			memcpy(&SectionHeader->VirtualAddress, &new_VirtualAddress, sizeof(DWORD));
			printSectionHeader(SectionHeader);
		}
	}
}

void ModifyRVADataDirectories(IMAGE_OPTIONAL_HEADER* OpHeader, int sizeFile) {
	DWORD new_RVA;
	for (int count = 0; count < OpHeader->NumberOfRvaAndSizes; count++) {
		if (OpHeader->DataDirectory[count].VirtualAddress != 0) {
			new_RVA = OpHeader->DataDirectory[count].VirtualAddress + sizeFile;
			memcpy(&OpHeader->DataDirectory[count].VirtualAddress, &new_RVA, sizeof(DWORD));
		}
	}
}

void ModOptionalHeader(IMAGE_NT_HEADERS* NtHeader, int sizeFile, int ImBase_originalPE) {
	IMAGE_FILE_HEADER* fHeader = &NtHeader->FileHeader;
	IMAGE_OPTIONAL_HEADER* OpHeader = &NtHeader->OptionalHeader;

	DWORD new_SizeOfUninitializedData = OpHeader->SizeOfUninitializedData + sizeFile;
	DWORD new_ImageBase = rounded_down(ImBase_originalPE - OpHeader->SizeOfImage, 0x10000);
	DWORD new_SizeOfImage = OpHeader->SizeOfImage + sizeFile;

	memcpy(&OpHeader->SizeOfUninitializedData, &new_SizeOfUninitializedData, sizeof(DWORD));
	memcpy(&OpHeader->ImageBase, &new_ImageBase, sizeof(DWORD));
	memcpy(&OpHeader->SizeOfImage, &new_SizeOfImage, sizeof(DWORD));
	ZeroMemory(&OpHeader->CheckSum, sizeof(DWORD));
}

void ModifyOptionalHeader(IMAGE_NT_HEADERS* NtHeader, int sizeFile) {
	IMAGE_FILE_HEADER* fHeader = &NtHeader->FileHeader;
	IMAGE_OPTIONAL_HEADER* OpHeader = &NtHeader->OptionalHeader;

	DWORD new_SizeOfUninitializedData = OpHeader->SizeOfUninitializedData + sizeFile;
	DWORD new_AddressOfEntryPoint = OpHeader->AddressOfEntryPoint + sizeFile;
	DWORD new_BaseOfCode = OpHeader->BaseOfCode + sizeFile;
	DWORD new_BaseOfData = OpHeader->BaseOfData + sizeFile;
	DWORD new_SizeOfImage = rounded(OpHeader->SizeOfImage + sizeFile, OpHeader->SectionAlignment);
	DWORD new_SizeOfHeaders =rounded(OpHeader->SizeOfHeaders + sizeof(IMAGE_SECTION_HEADER), OpHeader->FileAlignment);

	memcpy(&OpHeader->SizeOfUninitializedData, &new_SizeOfUninitializedData, sizeof(DWORD));
	memcpy(&OpHeader->AddressOfEntryPoint, &new_AddressOfEntryPoint, sizeof(DWORD));
	memcpy(&OpHeader->BaseOfCode, &new_BaseOfCode, sizeof(DWORD));
	memcpy(&OpHeader->BaseOfData, &new_BaseOfData, sizeof(DWORD));
	memcpy(&OpHeader->SizeOfImage, &new_SizeOfImage, sizeof(DWORD));
	memcpy(&OpHeader->SizeOfHeaders, &new_SizeOfHeaders, sizeof(DWORD));
	ZeroMemory(&OpHeader->CheckSum, sizeof(DWORD));

	ModifyRVADataDirectories(OpHeader, sizeFile);
}

int main() {
	std::streampos szOriginalFile;
	std::ifstream originalPE("C:\\Users\\carlos\\Desktop\\PEview.exe", std::ios::in | std::ios::binary | std::ios::ate);
	std::streampos sizeFileFinal;
	std::ifstream fileFinal("C:\\Users\\carlos\\Desktop\\PEview.exe", std::ios::in | std::ios::binary | std::ios::ate);

	if (fileFinal.is_open() && originalPE.is_open())
	{
		szOriginalFile = originalPE.tellg();
		char* rawDataOrig = new char[szOriginalFile]();

		sizeFileFinal = fileFinal.tellg();
		char* rawData = new char[sizeFileFinal]();

		originalPE.seekg(0, std::ios::beg);
		originalPE.read(rawDataOrig, sizeFileFinal);

		fileFinal.seekg(0, std::ios::beg);
		fileFinal.read(rawData, sizeFileFinal);

		IMAGE_DOS_HEADER* DOSHeaderOrig = PIMAGE_DOS_HEADER(rawDataOrig);
		IMAGE_NT_HEADERS* NtHeaderOrig = PIMAGE_NT_HEADERS(DWORD(rawDataOrig) + DOSHeaderOrig->e_lfanew);

		IMAGE_DOS_HEADER* DOSHeader = PIMAGE_DOS_HEADER(rawData);
		IMAGE_NT_HEADERS* NtHeader = PIMAGE_NT_HEADERS(DWORD(rawData) + DOSHeader->e_lfanew);

	if (NtHeader->Signature == IMAGE_NT_SIGNATURE && NtHeaderOrig->Signature == IMAGE_NT_SIGNATURE) // Check if image is a PE File.
		{	
			int sizeOriginalFile = rounded(szOriginalFile, (int) ((IMAGE_OPTIONAL_HEADER*)(&NtHeader->OptionalHeader)->FileAlignment)); //Redondeamos al valor superior mas cercano de FileAlignment
			AddNumOfSections(NtHeader); // Añade en IFH +1 al Number Of Sections
			//###### Add Section to the begining of sections ######
				//AddSection(rawData, DOSHeader, NtHeader, sizeOriginalFile); // Crea una nueva seccion al ppio y desplaza las demás
				//ModifyOptionalHeader(NtHeader, sizeOriginalFile); //Modifica os valores de AddOfEntrryPoint, BaseOfCode, BaseOfData, SizeOfUninitializedData, SizeOfImage
			//###### Add Section to the end of sections ######
				AddSectionToEnd(rawData, DOSHeader, NtHeader, sizeOriginalFile); // Crea una nueva seccion al ppio y desplaza las demás
				DWORD ImBase_originalPE = (DWORD)((IMAGE_OPTIONAL_HEADER*)(&NtHeaderOrig->OptionalHeader)->ImageBase); //Sacamos la ImageBase del PE original
				ModOptionalHeader(NtHeader, sizeOriginalFile, ImBase_originalPE);
		}
		originalPE.close();
		fileFinal.close();

		ofstream outFile("C:\\Users\\carlos\\Desktop\\PEviewHeadMod.exe", ios::out | ios::binary);
		outFile.write(rawData, sizeFileFinal);
		outFile.close();
	}
	int i; cin >> i;
}
