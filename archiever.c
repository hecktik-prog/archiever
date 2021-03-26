#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <fcntl.h>

#define size_block 512

//объявление прототипов функций
int FilePackFunc(char* FNAME, int metadata_file, int depth);
int DirPackFunc(char* DIRNAME, int metadata_file, int depth);
int pack(char* DIRNAME, int metadata_file, int depth);
int unpack(char* DIRNAME, char* archieve);

int main(int argc, char* argv[])
{
    char* fname = "archieve"; //файл архива
    int check = 0;            //переменная для проверки режима работы программы

    if (strcmp(argv[1],"pack") == 0)
        check = 1;
    else if (strcmp(argv[1],"unpack") == 0)
        check = 2;
    else
        printf("Введена неизвестная команда\n");

    //Архивация файлов и директорий
    if (check == 1)
    {
        //Создаем файл-архив
        int a = open(fname, O_RDWR | O_CREAT, S_IRUSR | S_IWUSR | S_IXUSR | S_IRGRP | S_IXGRP | S_IROTH | S_IWOTH);

        //Архивирхивация файлов и директорий
        if (pack(argv[2], a, 0) < 0)
        {
            printf("Заархивировано неуспешно.\n");
            close(a);
            return -1;
        }
        else
        {
            printf("Успешно заархивировано.\n");
            close(a);
            return 1;
        }
    }
    if (check == 2)
    {
        //Разархивация файлов и дирикторий
        if (unpack(argv[2], fname) < 0)
        {
            printf("Разархивировано неуспешно.\n");
            return -1;
        }
        else
        {
            printf("Успешно разархивированно.\n");
            return 1;
        }
    }
}

int FilePackFunc(char* FNAME, int metadata_file, int depth)
{
  struct stat info;             //структура, соддержащая информацию о файле
  long long size;               //размер файла
  long int namesize;            //размер имени файла
  char block[size_block];       //блоки для записи
  int nread = 0;                //число считанных байтов
  int in;                       //дескриптор входного файла
  int helper = 1;

  if ((in = open(FNAME, O_RDONLY)) == -1)
  {
      printf("pack: Ошибка открытия файла %s", FNAME);
      return -1;
  }

  //получение информации по файлу
  lstat(FNAME, &info);
  size = info.st_size;
  namesize = sizeof(FNAME);


  if(write(metadata_file, &namesize, sizeof(long int)) < 0)
  {
    printf("pack: Ошибка записи размера имени файла\n");
    return -2;
  }
  if(write(metadata_file, FNAME, namesize) < 0)
  {
    printf("pack: Ошибка записи имени файла\n");
    return -3;
  }
  if(write(metadata_file, &helper, sizeof(int)) < 0)
  {
    printf("pack: Ошибка записи вспомогательной переменной\n");
    return -4;
  }
  if(write(metadata_file, &depth, sizeof(int)) < 0)
  {
    printf("pack: Ошибка записи текущей глубины\n");
    return -5;
  }
  if(write(metadata_file, &size, sizeof(long long)) < 0)
  {
    printf("pack: Ошибка записи размера файла\n");
    return -6;
  }

  //запись данных файла
  while(nread=read(in, block, 1) > 0)
  {
    if(write(metadata_file, block, nread) < 0)
    {
      printf("pack: Ошибка записи данных файла\n");
      return -7;
    }
  }

  return 0;
}

int DirPackFunc(char* DIRNAME, int metadata_file, int depth)
{
  long int dirnamesize; //размер имени директории
  int helper = 0;


  dirnamesize = sizeof(DIRNAME);

  if(write(metadata_file, &dirnamesize, sizeof(long int)) < 0)
  {
    printf("pack: Ошибка записи размера имени директории\n");
    return -8;
  }
  if(write(metadata_file, DIRNAME, dirnamesize) < 0)
  {
    printf("pack: Ошибка записи имени директории\n");
    return -9;
  }
  if(write(metadata_file, &helper, sizeof(int)) < 0)
  {
    printf("pack: Ошибка записи вспомогательной переменной\n");
    return -10;
  }
  if(write(metadata_file, &depth, sizeof(int)) < 0)
  {
    printf("pack: Ошибка записи глубины\n");
    return -11;
  }

  return 0;
}

int pack(char* DIRNAME,int metadata_file, int depth)
{
  struct stat info;        //структура, соддержащая информацию о файле
  struct dirent* dirinfo;  //структура, соддержащая информацию о директории
  DIR* dir;                //определение переменной типа DIR для работы с директорией

  if((dir = opendir(DIRNAME)) == NULL)
  {
    printf("pack: Ошибка открытия директории %s", DIRNAME);
    return -12;
  }

  //получение информации по директории
  chdir(DIRNAME);
  dirinfo = readdir(dir);

  //пока в директории есть содержимое (файл/другая директория)
  while(dirinfo != NULL)
  {
    //если не текущий и не родительский каталоги
    if(strcmp(".", dirinfo->d_name) != 0 && strcmp("..", dirinfo->d_name) != 0)
    {
      //получение информации по файлу
      lstat(dirinfo->d_name, &info);

      //если директория
      if(S_ISDIR(info.st_mode))
      {
        //архивация директории
        if(DirPackFunc(dirinfo->d_name,metadata_file,depth)< 0)
        {
          printf("pack: Ошибка архивации директории.\n");
          return -13;
        }
        //рекурсивно опускаемся на уровень ниже
        pack(dirinfo->d_name, metadata_file, depth + 1);
        rmdir(dirinfo->d_name);
      }
      //если файл
      else
      {
        //архивация файла
        if(FilePackFunc(dirinfo->d_name, metadata_file, depth) < 0)
        {
          printf("pack: Ошибка архивации файла.\n");
          return -14;
        }
        remove(dirinfo->d_name);
      }
    }
    //получение информации по директории
    dirinfo=readdir(dir);
  }

  //на уровень выше
  chdir("..");
  closedir(dir);
  return 0;
}

int unpack(char* DIRNAME, char* archieve)
{
  char* FNAME;          //имя текущего файла
  char* CurDir;
  char* SavePtr;
  DIR* dir;             //определение переменной типа DIR для работы с директорией
  long long size;       //размер файла
  long int namesize;    //размер имени файла
  char block[size_block];
  int in;               //дескриптор архиватора
  int out;              //декриптор разархивированного файла
  int depth;
  int helper;
  int CurDepth = 0;     //текущая глубина
  int checker;

  in = open(archieve, O_RDONLY);
  if (in == -1)
  {
    printf("unpack: Ошибка открытия файла %s\n", archieve);
    return -14;
  }

  dir = opendir(DIRNAME);
  if(dir == NULL)
  {
    printf("unpack: Ошибка открытия директории %s\n", DIRNAME);
    return -15;
  }

  //переходим в директорию
  chdir(DIRNAME);
  
  while(read(in, &namesize, sizeof(long int)) > 0)
  {
    FNAME = (char*)malloc(namesize+1);
    if (read(in, FNAME, namesize) < 0)
    {
      printf("unpack: Ошибка считывания имени файлов \n");
      close(in);
      return -16;
    }

    if (read(in, &checker, sizeof(int)) < 0)
    {
      printf("unpack: Ошибка считывания вспомогательной переменной\n");
      close(in);
      return -17;
    }

    if(read(in, &depth, sizeof(int)) < 0)
    {
      printf("unpack: Ошибка считывания грубины \n");
      close(in);
      return -18;
    }
    //если текущая глубина меньше считанной, переходим в текущую директорию
    if(CurDepth < depth)
    {
       chdir(CurDir);
    }

    //иначе
    if(CurDepth > depth)
    {
       helper = depth;
       while((CurDepth - helper) > depth)
       {
         chdir("..");
         helper++;
       }
    }
    CurDepth = depth;

    //cоздаем директорию
    if(checker == 0)
    {
      mkdir(FNAME, S_IRUSR|S_IWUSR|S_IXUSR|S_IRGRP|S_IWGRP|S_IXGRP|S_IROTH|S_IWOTH|S_IXOTH);
      CurDir = FNAME;
    }

    if(checker == 1)
    {
      if(read(in, &size, sizeof(long long)) < 0)
      {
        printf("unpack: Ошибка считывания размера файла \n");
        close(in);
        return -19;
      }

      if(read(in, block, size) < 0)
      {
        printf("unpack: Ошибка считывания данных файла \n");
        close(in);
        return -20;
      }

      out = open(FNAME, O_WRONLY|O_CREAT, S_IRUSR|S_IWUSR|S_IXUSR|S_IRGRP|S_IXGRP|S_IROTH|S_IWOTH);

      if(write(out, block, size) < 0)
      {
        printf("unpack: Ошибка записи данных в файл \n");
        close(in);
        return -21;
      }
    }
  }
  close(in);
  return 1;
}
