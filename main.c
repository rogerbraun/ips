#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>


int read_chunk(FILE *file) {
  uint8_t buffer[3];

  fread(buffer, sizeof(uint8_t), 3, file);
  printf("%s\n", buffer);

  if(strncmp(buffer, "EOF", 3) == 0) {
    printf("End of File\n");
    return 1;
  }

  uint32_t offset = 0;
  offset += (buffer[0] << 16);
  offset += (buffer[1] << 8);
  offset += buffer[2] ;

  printf("Offset: %x\n", offset);

  fread(buffer, sizeof(uint8_t), 2, file);

  uint16_t length = 0;
  length += (buffer[0] << 8);
  length += buffer[1] ;

  printf("Length: %x\n", length);

  if(length == 0) {
    printf("RLE encoded chunk...\n");

    fread(buffer, sizeof(char), 2, file);

    length = 0;
    length += (buffer[0] << 8);
    length += buffer[1] ;
    printf("Real length: %u\n", length);
    fseek(file, 1, SEEK_CUR);

  } else {
    fseek(file, length, SEEK_CUR);
  }

  return 0;
}

int patch_chunk(FILE *base_rom, FILE *ips, FILE *outfile) {
  uint8_t buffer[3];

  fread(buffer, sizeof(uint8_t), 3, ips);

  if(strncmp(buffer, "EOF", 3) == 0) {
    printf("End of File\n");
    return 1;
  }

  uint32_t offset = 0;
  offset += (buffer[0] << 16);
  offset += (buffer[1] << 8);
  offset += buffer[2] ;

  printf("Offset: %x\n", offset);
  fseek(outfile, offset, SEEK_SET);

  fread(buffer, sizeof(uint8_t), 2, ips);

  uint16_t length = 0;
  length += (buffer[0] << 8);
  length += buffer[1] ;

  printf("Length: %x\n", length);

  if(length == 0) {
    printf("RLE encoded chunk...\n");

    fread(buffer, sizeof(char), 2, ips);

    length = 0;
    length += (buffer[0] << 8);
    length += buffer[1] ;
    printf("Real length: %u\n", length);
    int i;
    uint8_t single_buffer;

    single_buffer = fgetc(ips);
    for(i = 0; i < length; i++) {
      fputc(single_buffer, outfile);
    }

  } else {

    int i;
    uint8_t single_buffer;
    for(i = 0; i < length; i++) {
      single_buffer = fgetc(ips);
      fputc(single_buffer, outfile);
    }
  }

  return 0;
}

void load_ips(char *filename) {
  printf("Loading %s\n", filename);

  FILE *ips_file;
  char buffer[6];

  buffer[5] = 0;

  const char* MAGIC = "PATCH";

  ips_file = fopen(filename, "rb");
  fread(buffer, sizeof(char), 5, ips_file);
  printf("%s\n", buffer);

  if(strncmp(MAGIC, buffer, 5) == 0) {
    printf("This is an IPS patch\n");
  } else {
    printf("No idea what this is...\n");
    return;
  }

  int end = 0;
  while(!end) {
    end = read_chunk(ips_file);
  }
}

bool is_ips_file(FILE *possible_ips_file) {
  uint8_t buffer[5];
  fread(buffer, sizeof(uint8_t), 5, possible_ips_file);

  return (strncmp("PATCH", buffer, 5) == 0);
}

void patch_file(char *base_rom_name, char *ips_name, char *outfile_name) {
  FILE *base_rom, *ips, *outfile;

  ips = fopen(ips_name, "rb");
  // TODO: Check error

  // Check if the ips file is even valid.

  if(is_ips_file(ips)) {
    printf("Looks like an IPS file, patching...\n");
  } else {
    printf("Not an IPS file, exiting.\n");
    exit(1);
  }

  // Open the base rom
  base_rom = fopen(base_rom_name, "rb");
  // TODO: Check error

  // Open the destination file
  outfile = fopen(outfile_name, "wb");
  // TODO: Check error

  // Copy base rom to destination
  size_t base_rom_size;

  // C smartness that figures out the file size
  fseek(base_rom, 0, SEEK_END);
  base_rom_size = ftell(base_rom);
  rewind(base_rom);

  printf("File is %d bytes long...", base_rom_size);

  // Copy base_rom to outfile

  uint8_t buf;

  int i;

  for(i = 0; i < base_rom_size; i++) {
    buf = fgetc(base_rom);
    fputc(buf, outfile);
  }

  //  Do the actual patching..

  rewind(base_rom);
  rewind(outfile);

  int end = 0;
  while(!end) {
    end = patch_chunk(base_rom, ips, outfile);
  }

  fclose(ips);
  fclose(base_rom);
  fclose(outfile);
}


int main(int argc, char *argv[]) {
  if(argc == 2) {
    load_ips(argv[1]);
  }

  if(argc == 4) {
    printf("Patching file...\n");
    patch_file(argv[1], argv[2], argv[3]);
  }
}
