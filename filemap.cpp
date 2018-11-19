/*!
 *
 * \file filemap.cpp
 * \brief реализация класса проекция файла в память
 *
 *  объект поддерживает большие файлы ( >4Gb )\n
 *  функционал позволяет проецировать файл частично, не целиком\n
 *  текстовые файлы может считывать построчно
 *
 * Copyright (C) 2018 Pochepko PP.
 * Contact: ppp.it@hotmail.com
 *
 * This file is part of software written by Pochepko PP.
 *
 * This software is provided 'as-is', without any express or implied
 * warranty. In no event will the authors be held liable for any damages
 * arising from the use of this software.
 *
 * Permission is granted to anyone to use this software for any purpose,
 * including commercial applications, and to alter it and redistribute it
 * freely, subject to the following restrictions:
 *
 *    1. The origin of this software must not be misrepresented; you must not
 *    claim that you wrote the original software. If you use this software
 *    in a product, an acknowledgment in the product documentation would be
 *    appreciated but is not required.
 *
 *    2. Altered source versions must be plainly marked as such, and must not be
 *    misrepresented as being the original software.
 *    3. This notice may not be removed or altered from any source
 *    distribution.
 *
 */
//-!-!-!-!-!-!-!-!-!-!-!-!-!-!-!-!-!-!-!-!-!-!-!-!-!-!-!-!-!-!-!-!-!-!-!-!-!-!-!-!-!-!-!-!-!-!-!-!-!-!


#include "filemap.h"
#include <iostream>
#include <locale>       /* std::wstring_convert */
#include <codecvt>      /* std::codecvt_utf8_utf16 */

using namespace std;

///////////////////////////////////////////////////////////////////////////////
// конструктор
CFileMap::CFileMap( uint64_t limit_map_memory /*= 0*/ )
{

    m_offset.QuadPart = 0;          //  смещение от начала файла
    m_offset_block = 0;             //  смещение от начала текущего блока проекции
    m_address.map_mth = 0;          // адрес, куда отображается файл (изменяемый в процессе обработки)
    m_file = INVALID_HANDLE_VALUE;  // описатель файла
#   if ( defined(OS_LINUX) || defined(OS_UNUX) )
    m_map_mode = MAP_PRIVATE;       // режим доступа к проекции
    m_page_protect = PROT_READ;     // режим доступа к страницам памяти
#   else
    m_map_mode = FILE_MAP_READ;     // режим доступа к проекции
    m_hFileMapping = INVALID_HANDLE_VALUE;
#   endif  // defined(OS_WIN)
    m_file_path.clear();            // полное имя файла
    m_file_size.QuadPart = 0;       // размер файла
    m_max_copy = 0;                 //  максимальное количество байт для копирования
    m_ptr_file = nullptr;           // адрес, куда отображается файл (неизменяемый - для освобождения)
    m_page_size = get_page_size();  // размер страницы памяти в OS
    m_limit_memory = 0;
    set_limit_memory( limit_map_memory );
    m_sync = true;
#if defined(OS_WIN)
    strcpy( m_new_line, "\r\n" );
    m_return = false;
#else
    strcpy( m_new_line, "\n" );
#endif  // defined(OS_WIN)

}   //  CFileMap()

///////////////////////////////////////////////////////////////////////////////
// деструктор
CFileMap::~CFileMap()
{
#   ifdef check_destuctor
    cout<<"CFileMap::~CFileMap()."<<endl;
#   endif // check_destuctor
    close_file_map();
}   //  ~CFileMap()

///////////////////////////////////////////////////////////////////////////////
// открыть файл для последющего отображения используя флаги
#if defined(OS_WIN)
uint32_t CFileMap::open_file_map(uint32_t md_fl /*=GENERIC_READ*/,         /* mode file access      */
                                 uint32_t md_sh /*=SHARE_DENIED*/,         /* mode share access     */
                                 uint32_t md_op /*=OPEN_EXISTING*/,        /* mode file open/create */
                                 uint32_t md_at /*=FILE_ATTRIBUTE_NORMAL*/,/* flag file attribute   */
                                 uint32_t md_pp /*=PAGE_READONLY*/,        /* mode page protect     */
                                 uint32_t md_mm /*=FILE_MAP_READ*/,        /* mode map access       */
                                 uint64_t offset /*=0*/ )                  /* смещение от начала    */
{
    uint32_t last_error = 0;
    m_return = false;
    m_offset.QuadPart = offset;

    if ( md_fl == GENERIC_READ )
        m_sync = false;

    // проверим инициализированы ли критически важные переменные
    if ( m_file_size.QuadPart == 0 || m_file_path.length() == 0 || m_page_size == 0 ) {
        return ERROR_INVALID_PARAMETER;
    }

    m_map_mode = md_mm;

    // открываем файл, который мы собираемся отобразить в память
    /* If the function succeeds, the return value is an open handle to the specified file,
     * device, named pipe, or mail slot.
     * If the function fails, the return value is INVALID_HANDLE_VALUE.
     * To get extended error information, call GetLastError. */
    m_file = ::CreateFileW( m_file_path.c_str(), //  PCSTR pszFileName,
                            md_fl,               //  Режим доступа к файлу
                            md_sh,               /*  Режим совместного доступа к файлу.*/
                            NULL,                //  PSECURITY_AIIRIBUTES psa,
                            md_op,               /*  Открыть и/или создать/обрезать файл */
                            md_at,               //  Флаги атрибутов файла
                            NULL                 //  HANDLE hTemplateFile
                            );    // CloseHandle( m_file );

    if ( m_file == INVALID_HANDLE_VALUE ) {
        last_error = ::GetLastError();
        return last_error;
    }

    // объект "проекция файла"
    /* If the function succeeds, the return value is a handle to the newly created file
     * mapping object. If the object exists before the function call, the function returns
     * a handle to the existing object (with its current size, not the specified size),
     * and GetLastError returns ERROR_ALREADY_EXISTS. If the function fails, the return
     * value is NULL. To get extended error information, call GetLastError. */
    m_hFileMapping = ::CreateFileMapping(
                m_file,                 //  HANDLE hFile,
                NULL,                   //  PSECURITY_ATTRIBUTES psa,
                md_pp,                  //  DWORD fdwProtect,
                m_file_size.HighPart,   /*  Старшее слово (DWORD) максимального размера объекта.    */
                m_file_size.LowPart,    /*  Младшее слово (DWORD) максимального размера объекта.
                                         *  Если этот и предыдущий параметры равняются нулю,
                                         *  размер объекта равен текущему размеру файла.            */
                NULL                    //  PCSTR pszName
                ); //  CloseHandle( m_hFileMapping );


    if ( m_hFileMapping == NULL ) {
        last_error = ::GetLastError();
        return last_error;
    }

    if ( last_error == 0 ) {
        last_error = map_region( offset );
    }

    return last_error;
}   //  open_file_map ( uint32_t md_fl, uint32_t md_sh, ...
#else
uint32_t CFileMap::open_file_map(uint32_t md_fl /*=O_RDONLY*/,             /* mode file access      */
                                 uint32_t md_op /*=NO_FLAG*/,              /* mode file open/create */
                                 uint32_t md_pp /*=PROT_READ*/,            /* mode page protect     */
                                 uint32_t md_mm /*=MAP_PRIVATE*/,          /* mode map access       */
                                 uint64_t offset /*=0*/,                   /* смещение от начала    */
                                 mode_t mode /*=S_IRWXU|S_IRWXG|S_IROTH*/) /* задает права доступа  */
{
    uint32_t last_error = 0;
    m_offset.QuadPart = offset;

    if ( md_fl == O_RDONLY )
        m_sync = false;

    // проверим инициализированы ли критически важные переменные
    if ( m_file_size.QuadPart == 0 || m_file_path.length() == 0 || m_page_size == 0 ) {
        return EINVAL;
    }

    m_map_mode = md_mm;
    m_page_protect = md_pp;

    string file_path = wchar_string( m_file_path.c_str(), m_file_path.length() );
    /* возвращают новый описатель файла или -1 в случае ошибки (в этом случае
     * значение переменной errno устанавливается должным образом). */
    m_file = ::open( file_path.c_str(), md_fl | md_op | O_LARGEFILE, mode );
    if ( m_file == INVALID_HANDLE_VALUE ) {
        last_error = errno;
        return last_error;
    }

    if ( (md_fl & O_WRONLY) == O_WRONLY || (md_fl & O_RDWR) == O_RDWR ) {
        /* Если мы не установим размер выходного файла таким способом, функции mmap
         * завершится успехом, но при первой же попытке обратиться к отображенной
         * памяти мы получим сигнал SIGBUS. */
        int result = ::lseek( m_file, m_file_size.QuadPart-1, SEEK_SET );
        if ( result == INVALID_HANDLE_VALUE ) {
            last_error = errno;
            return last_error;
        }
        result = ::write( m_file, "", 1 );
        if ( result == INVALID_HANDLE_VALUE ) {
            last_error = errno;
            return last_error;
        }
    }

    if ( last_error == 0 ) {
        last_error = map_region( offset );
    }

    return last_error;;
}   //  open_file_map ( uint32_t md_fl, uint32_t md_sh, ...
#endif  // defined(OS_WIN)

///////////////////////////////////////////////////////////////////////////////
// открыть файл для последющего отображения используя предопределенный режим
uint32_t CFileMap::open_file_map ( mode md, uint64_t offset /*= 0*/ )
{
    uint32_t last_error = 0;

#   if defined(OS_WIN)
    uint32_t md_fl = GENERIC_READ;           /* mode file access      */
    uint32_t md_sh = SHARE_DENIED;           /* mode share access     */
    uint32_t md_op = OPEN_EXISTING;          /* mode file open/create */
    uint32_t md_at = FILE_ATTRIBUTE_NORMAL;  /* flag file attribute   */
    uint32_t md_pp = PAGE_READONLY;          /* mode page protect     */
    uint32_t md_mm = FILE_MAP_READ;          /* mode map access       */
#   else
    uint32_t md_fl = O_RDONLY;               /* mode file access      */
    uint32_t md_op = NO_FLAG;
    uint32_t md_pp = PROT_READ;              /* mode page protect     */
    uint32_t md_mm = MAP_PRIVATE;            /* mode map access       */
#   endif  // defined(OS_WIN)

    try
    {
        switch ( md ) {
        case mode::read:
            /* сам файл, страницы памяти и проекция файла с доступом только чтение.
             *  Другим потокам/процесам разрешен доступ на чтение.
             *  Файл обязательно должен существовать. */
#           if defined(OS_WIN)
            md_sh = FILE_SHARE_READ;         /* Разрешает последующие операции открытия для чтения */
#           endif  // defined(OS_WIN)
            break;

        case mode::write:
            /* Доступ на запись для файла, страниц памяти и объекта проекции.
             *  Другим потокам/процесам запрещен доступ к файлу.
             *  если файл не существует - то создается,
             *  если существует - то размер усекается до нуля. */
#           if defined(OS_WIN)
            md_fl = GENERIC_READ | GENERIC_WRITE;
            md_op = CREATE_ALWAYS;         /* Создает новый. Если существует, то переписывает файл */
            md_pp = PAGE_READWRITE;        /* Дает доступ к операциям чтения-записи страниц        */
            md_mm = FILE_MAP_WRITE;        /* Доступ к операциям чтения-записи.                    */
#           else
            md_fl = O_RDWR;                /* файл доступен для записи                             */
            md_op = O_CREAT | O_TRUNC;     /* Создает новый. Если существует, то переписывает файл */
            md_pp = PROT_WRITE | PROT_READ;/* можно записывать информацию                          */
            md_mm = MAP_SHARED;            /* Доступ к операциям чтения-записи.                    */
#           endif  // defined(OS_WIN)
            break;

        case mode::append:
            /* Доступ на запись для файла, страниц памяти и объекта проекции.
             *  Другим потокам/процесам разрешен доступ на чтение.
             *  Файл обязательно должен существовать. */
#           if defined(OS_WIN)
            md_fl = GENERIC_READ | GENERIC_WRITE;
            md_sh = FILE_SHARE_READ;       /* Разрешает последующие операции открытия для чтения   */
            md_op = OPEN_EXISTING;         /* Функция завершается ошибкой, если файл не существует */
            md_pp = PAGE_READWRITE;        /* Дает доступ к операциям чтения-записи страниц        */
            md_mm = FILE_MAP_WRITE;        /* Доступ к операциям чтения-записи.                    */
#           else
            md_fl = O_RDWR;                /* файл доступен для чтения и записи                    */
            md_op = O_APPEND;              /* Файл открывается в режиме добавления                 */
            md_pp = PROT_WRITE | PROT_READ;/* можно записывать информацию                          */
            md_mm = MAP_SHARED;            /* Доступ к операциям чтения-записи.                    */
#           endif  // defined(OS_WIN)
            break;
        }
#       if defined(OS_WIN)
        last_error = open_file_map( md_fl, md_sh, md_op, md_at, md_pp, md_mm, offset );
#       else
        last_error = open_file_map( md_fl, md_op, md_pp, md_mm, offset );
#       endif  // defined(OS_WIN)
        if ( last_error ) {
            throw last_error;
        }
    }
    catch( uint32_t error ) {
        last_error = error;
        cout<< "an error number \"" << error << "\" is generated in the method open_file_map" <<endl;
    }

    return last_error;;
}   //  open_file_map ( mode md, uint64_t offset /*= 0*/ )

///////////////////////////////////////////////////////////////////////////////
// отражает файл (часть файла) в память
uint64_t CFileMap::map_region ( uint64_t offset /*= 0*/, uint64_t size_region /*= 0*/ )
{
    uint32_t last_error = 0;

    // если отражение было выполнено - освободим память
    if ( m_ptr_file ) {
        last_error = unmap_region( m_limit_memory );
        if ( last_error ) {
            throw last_error;
        }
    }

    if ( size_region == 0 ) {
        size_region = m_limit_memory;
    } else {
        m_limit_memory = size_region;
    }

    if ( offset ) { // смещение от начала файла
        m_offset.QuadPart = offset;
    }
    m_offset_block = 0; // смещение от начала текущего блока

    try
    {
        // отображаем файл в память
#   if defined(OS_WIN)
        /* If the function succeeds, the return value is the starting address of the mapped view.
         * If the function fails, the return value is NULL. To get extended error information,
         * call GetLastError. */
        m_ptr_file = ::MapViewOfFile( m_hFileMapping,     //  HANDLE hFileMappingObject,
                                      m_map_mode,         //  Режим доступа к проекции
                                      m_offset.HighPart,  /*  Старшее двойное слово (DWORD) смещения
                                                           *  файла, где начинается отображение. */
                                      m_offset.LowPart,   /*  Младшее двойное слово (DWORD) смещения
                                                           *  файла, где начинается отображение. */
                                      size_region         /*  Число отображаемых байтов файла.
                                                           *  Если == 0, отображается весь файл.*/
                                      );

        if ( m_ptr_file == NULL ) {
            last_error = ::GetLastError();
            throw last_error;
        }
#   else
        if ( size_region == 0 )
            size_region = m_file_size.QuadPart;

        /* On success, mmap() returns a pointer to the mapped area.
         * On error, the value MAP_FAILED (that is, (void *) -1) is returned,
         * and errno is set to indicate the cause of the error. */
        m_ptr_file = ::mmap( nullptr, size_region, m_page_protect,
                             m_map_mode, m_file, m_offset.QuadPart );

        if ( m_ptr_file == MAP_FAILED ) {
            last_error = errno;
            throw last_error;
        }
#   endif  // defined(OS_WIN)
    }
    catch( uint32_t error ) {
        cout<< "an error number \"" << error << "\" is generated in the method map_region" <<endl;
        m_ptr_file = nullptr;
    }

    set_max_copy();

    /* m_ptr_file нужен для освобожнения региона, т.к.  m_address.map_ptr будет изменятся
     * в процессе выполнения */
    m_address.map_ptr = m_ptr_file;

    return last_error;
}   //  map_region ( uint64_t size_region /*= 0*/ )

///////////////////////////////////////////////////////////////////////////////
// снимет отражение в памяти (освобождает память)
uint32_t CFileMap::unmap_region ( uint64_t size_region, bool sync /*= true*/ )
{
    uint32_t last_error = 0;

    if ( size_region == 0 )
        size_region = m_file_size.QuadPart;
//    else
//        // выравнивание размера отображения файла с учетом гранулярности страниц памяти
//        size_region = memory_allocation_granularity( size_region );

#   if defined(OS_WIN)
    try
    {
        if ( m_ptr_file ) {
            BOOL bError = FALSE;

            // синхронизируем проекцию с файлом (освобожденные страницы памяти записываются на диск)
            if ( sync == true && m_sync == true  ) {
                /* Writes to the disk a byte range within a mapped view of a file.
                 * If the function succeeds, the return value is nonzero.
                 * If the function fails, the return value is zero.
                 * To get extended error information, call GetLastError. */
                bError = ::FlushViewOfFile( m_ptr_file, size_region );
                if ( bError == FALSE ) {
                    uint32_t last_error = ::GetLastError();
                    throw last_error ;
                }
            }

            bError = FALSE;
            /* Если функция завершается успешно, возвращаемое значение не нуль, а все
             * недействительные страницы, внутри заданной области "вяло" записываются  на диск.
             * Если функция завершается ошибкой, возвращаемое значение равняется нулю.
             * Чтобы получить дополнительную информацию об ошибке, вызовите GetLastError.  */
            bError = ::UnmapViewOfFile( m_ptr_file );
            m_ptr_file = nullptr;
            m_address.map_ptr = m_ptr_file;
            if ( bError == FALSE ) {
                uint32_t last_error = ::GetLastError();
                throw last_error ;
            }
        }
    }
    catch( uint32_t error ) {
        last_error = error;
        cout<< "an error number \"" << error << "\" is generated in the method unmap_region" <<endl;
    }
#   else
    try
    {
        if ( m_ptr_file ) {

            int res = 0;
            // синхронизируем проекцию с файлом (освобожденные страницы памяти записываются на диск)
            if ( sync == true && m_sync == true ) {
                /* При удачном завершении вызова возвращаемое значение равно нулю.
                 * При ошибке оно равно -1, а переменной errno присваивается номер ошибки. */
                res = ::msync( m_ptr_file, size_region, MS_ASYNC );
                if ( res ) {
                    uint32_t last_error = errno;
                    throw last_error ;
                }
            }

            res = 0;
            /* При удачном выполнении munmap возвращаемое значение равно нулю. При ошибке
             * возвращается -1, а переменная errno приобретает соответствующее значение.
             * (Вероятнее всего, это будет EINVAL).  */
            res = ::munmap( m_ptr_file, size_region );
            m_ptr_file = nullptr;
            m_address.map_ptr = m_ptr_file;
            if ( res ) {
                uint32_t last_error = errno;
                throw last_error ;
            }
        }
    }
    catch( uint32_t error ) {
        cout<< "an error number \"" << error << "\" is generated in the method unmap_region" <<endl;
    }
#   endif  // defined(OS_WIN)

    return last_error;
}   //  unmap_region ( uint64_t size_region, bool sync /*= true*/ )

///////////////////////////////////////////////////////////////////////////////
// записать строку в файл
uint32_t CFileMap::write( const char *str, size_t length )
{
    if ( eof() )
        return 0;
    // проверим, сколько байт можно записать
    if ( m_max_copy == 0 ) {
        if ( next_region() != 0 )
            return 0;
    }

    // счетчик скопированных байт
    size_t copy2file = 0;

    // проверим, сколько байт можно записать/прочитать
    if ( length <= m_max_copy ) {
        // операция может быть выполнена целиком
        copy2file = write2memory( str, length );
    } else {
        /* в текущей проекции недостаточно места для выполнения операции
         * поэтому если файл открыт целиком - то просто скопируем только
         * то, что позволяет размер файла, иначе скопируем чать в текущую
         * проекцию, а потом откроем новую и скопируем оставшуюся часть */
        if ( m_limit_memory == 0 ) {
            // файл открыт целиком
            copy2file = write2memory( str, m_max_copy );
        } else {
            // файл открыт в режиме блочного доступа
            while ( length > m_max_copy ) {
                size_t copied = write2memory( str+copy2file, m_max_copy );
                length = length - copied;
                copy2file = copy2file + copied;
                if ( eof() )
                    break;

                // определим размер блока для проекции, что бы не выйти за границу файла
                uint64_t size_region = m_file_size.QuadPart - m_offset.QuadPart;
                if ( size_region < m_limit_memory ) {
                    if ( map_region( 0, size_region ) != 0 )
                        return 0;
                } else {
                    if ( map_region() != 0 )
                        return 0;
                }
            }
            // проверим что все скоировано
            if ( length && !eof() ) {
                copy2file = copy2file + write2memory( str+copy2file, length );
            }
        }
    }
    return copy2file;
}   //  write( const char *str, size_t length )

///////////////////////////////////////////////////////////////////////////////
// прочитать строку из файла
uint32_t CFileMap::read_line( char *dest )
{
    // указатель на проекцию очередного блока файла
    const char *file = nullptr;
    if ( eof() )
        return 0;
    // проверим, сколько байт можно прочитать
    if ( m_max_copy == 0 ) {
        if ( next_region() != 0 )
            return 0;

#       if defined(OS_WIN)
        /* в *nix систмах символ перехода на новую строку занимает 1 байт,
         * поэтому алгоритм все отработает, а вот в Windows нужно проверить
         * первый символ и учеть последний байт предыдущего диапазона  */
        if ( m_return == false && m_max_copy == 1 ) {
            // просто прочитаем символ
            return read( dest, 1 );
        }
        // указатель на проекцию очередного блока файла
        file = (const char *)m_address.map_ptr;
        // проверим что первый символ - конец строки
        if ( (m_return && !strncmp( file, m_new_line+1, 1 )) ) {
            // переход на строку разбит между поддиапазонами
            m_return = false;
            return read( dest, 1 )-sizeof(m_new_line);
        } else {
            m_return = false;
        }
#       endif  // defined(OS_WIN)
    }

    // флаг true - нашли симвло(ы) перехода на новую строку
    bool find = false;
    // указатель на проекцию очередного блока файла
    file = (const char *)m_address.map_ptr;
    // счетчик скопированных байт
    uint32_t length = sizeof(m_new_line);

    // найдем символ новой строки
    while ( length <= m_max_copy ) {
        if ( !strncmp( file, m_new_line, sizeof(m_new_line) ) ){
            find = true;
            break;
        }

        file = file + 1;
        length = length+ 1;
    }

    if ( find == false ) {
        length = read( dest, m_max_copy );
        dest = dest + length;
#       if defined(OS_WIN)
        if ( m_max_copy == 0 )
            m_return = !strncmp( file, m_new_line, 1 );
#       endif  // defined(OS_WIN)
        length = length + read_line( dest );
    } else {
        length = read( dest, length );
        length = length-sizeof(m_new_line);
    }

    return length;
}   //  read_line( const char *dest )

///////////////////////////////////////////////////////////////////////////////
// прочитать данные из файла
uint32_t CFileMap::read( char *dest, size_t length )
{
    if ( eof() )
        return 0;
    // проверим, сколько байт можно прочитать
    if ( m_max_copy == 0 ) {
        if ( next_region() != 0 )
            return 0;
    }
    if ( length == 0 )
        return 0;

    // счетчик скопированных байт
    size_t copy_from_file = 0;

    // проверим, сколько байт можно записать/прочитать
    if ( length <= m_max_copy ) {
        // операция может быть выполнена целиком
        copy_from_file = read_from_memory( dest, length );
    } else {
        /* в текущей проекции недостаточно места для выполнения операции
         * поэтому если файл открыт целиком - то просто скопируем только
         * то, что позволяет размер файла, иначе скопируем чать из текущей
         * проекции, а потом откроем новую и скопируем оставшуюся часть */
        if ( m_limit_memory == 0 ) {
            // файл открыт целиком
            copy_from_file = read_from_memory( dest, m_max_copy );
        } else {
            // файл открыт в режиме блочного доступа
            while ( length > m_max_copy ) {
                size_t copied = read_from_memory( dest+copy_from_file, m_max_copy );
                length = length - copied;
                copy_from_file = copy_from_file + copied;
                if ( eof() )
                    break;

                if ( next_region() != 0 )
                    return 0;
            }
            // проверим что все скоировано
            if ( length && !eof() ) {
                copy_from_file = copy_from_file + read_from_memory( dest+copy_from_file, length );
            }
        }
    }
    return copy_from_file;
}   //  read( const char *dest, size_t length )

///////////////////////////////////////////////////////////////////////////////
// проверка выделеного региона и проекция следующего
void* CFileMap::check_map_region( uint64_t length )
{
    if ( length == 0 )
        return m_address.map_ptr;

    m_address.map_mth += length;

    set_max_copy( length );
    if ( eof() )
        return 0;
    // проверим, сколько байт можно прочитать
    if ( m_max_copy == 0 ) {
        if ( next_region() != 0 )
            return 0;
    }
    return m_address.map_ptr;
}   //  check_map_region( uint64_t length )

///////////////////////////////////////////////////////////////////////////////
// отразить в память следующую часть файла
uint32_t CFileMap::next_region()
{
    // определим размер блока для проекции, что бы не выйти за границу файла
    uint64_t size_region = m_file_size.QuadPart - m_offset.QuadPart;
    if ( size_region < m_limit_memory ) {
        return map_region( 0, size_region );
    } else {
        return map_region();
    }
}   //  next_region()

///////////////////////////////////////////////////////////////////////////////
// установить максимальное количество байт доступных для чтения/записи
void CFileMap::set_max_copy( size_t length /*= 0*/ )
{
    // проверим что есть проекция
    if ( m_ptr_file == 0 ) {
        m_max_copy = 0;
        return;
    }
    // если передано количество прочитанных/записанных байт
    if ( length > 0 ) {
        m_offset_block += length;
        m_offset.QuadPart += length;
    }
    if ( m_limit_memory != 0 ) // используется блочный режим отражения в память
        m_max_copy = m_limit_memory - m_offset_block;
    else // файл целиком отражен в память
        m_max_copy = m_file_size.QuadPart - m_offset.QuadPart;
}   //  set_max_copy( size_t length /*= 0*/ )

///////////////////////////////////////////////////////////////////////////////
// копирует length байт из src_ptr в m_address.map_ptr
size_t CFileMap::write2memory ( const void *src_ptr, size_t length )
{
#   if defined(__STDC_LIB_EXT1__) || defined(OS_WIN)
    int res = memcpy_s( m_address.map_ptr, m_max_copy, src_ptr, length );
    if ( res == 0 ) {
        m_address.map_mth = m_address.map_mth + length;
        set_max_copy( length );
        return length;
    } else {
        return 0;
    }
#   else
    void* res = memcpy( m_address.map_ptr, src_ptr, length );
    if ( res ) {
        m_address.map_mth = m_address.map_mth + length;
        set_max_copy( length );
        return length;
    } else {
        return 0;
    }
#   endif
}   //  write2memory ( void *src_ptr, uint64_t number_of_bytes )

///////////////////////////////////////////////////////////////////////////////
// копирует length байт из m_address.map_ptr в dest_ptr
size_t CFileMap::read_from_memory ( void *dest_ptr, size_t length )
{
#   if defined(__STDC_LIB_EXT1__) || defined(OS_WIN)
    int res = memcpy_s( dest_ptr, m_max_copy, m_address.map_ptr, length );
    if ( res == 0 ) {
        m_address.map_mth = m_address.map_mth + length;
        set_max_copy( length );
        return length;
    } else {
        return 0;
    }
#   else
    void* res = memcpy( dest_ptr, m_address.map_ptr, length );
    if ( res ) {
        m_address.map_mth = m_address.map_mth + length;
        set_max_copy( length );
        return length;
    } else {
        return 0;
    }
#   endif
}   //  read_from_memory ( void *dest_ptr, size_t length )

///////////////////////////////////////////////////////////////////////////////
// закрывает объект
void CFileMap::close_file_map ( bool b_shrink_to_fit /*= false*/ )
{
    try
    {

        if ( m_ptr_file ) {
            uint32_t res = unmap_region( m_limit_memory );
            m_limit_memory = 0;
            if ( res ) {
                throw res ;
            }
        }
        m_limit_memory = 0;

#       if defined(OS_WIN)

        m_return = false;
        BOOL bError = FALSE;
        if ( m_hFileMapping != INVALID_HANDLE_VALUE ) {
            // If the function succeeds, the return value is nonzero.
            // If the function fails, the return value is zero.
            // To get extended error information, call GetLastError.
            bError = !::CloseHandle( m_hFileMapping );
        }
        m_hFileMapping = INVALID_HANDLE_VALUE;
        if ( bError ) {
            uint32_t last_error = ::GetLastError();
            throw last_error;
        }

        if ( b_shrink_to_fit == true ) {
            shrink_to_fit();
        }

        bError = FALSE;
        if ( m_file != INVALID_HANDLE_VALUE ) {
            // If the function succeeds, the return value is nonzero.
            // If the function fails, the return value is zero.
            // To get extended error information, call GetLastError.
            bError = !::CloseHandle( m_file );
        }
        m_file = INVALID_HANDLE_VALUE;
        if ( bError ) {
            uint32_t last_error = ::GetLastError();
            throw last_error ;
        }
#       else
        int res = 0;

        if ( b_shrink_to_fit == true ) {
            shrink_to_fit();
        }

        if ( m_file != INVALID_HANDLE_VALUE ) {
            // После успешного выполнения close возвращаемое значение
            // становится равным нулю, а в случае ошибки оно равно -1.
            res = ::close( m_file );
        }
        m_file = INVALID_HANDLE_VALUE;

        if ( res ) {
            uint32_t last_error = errno;
            throw last_error ;
        }
#       endif  // defined(OS_WIN)

    }
    catch( uint32_t error ) {
        cout <<"an error number \""<< error <<"\" is generated in the method close" <<endl;
    }
}   //  close_file_map ( bool b_shrink_to_fit /*= false*/ )

///////////////////////////////////////////////////////////////////////////////
// подогнать газмер файла под размер данных
void CFileMap::shrink_to_fit()
{
    try
    {

#       if defined(OS_WIN)
        if ( m_offset.QuadPart < m_file_size.QuadPart ) {

            /* Если функция выполнена успешно, возвращаемое значение отличное от нуля.
             * Если функция не работает, возвращаемое значение равно нулю.
             * Чтобы получить расширенную информацию об ошибке, вызовите GetLastError. */
            if ( ::SetFilePointerEx( m_file, m_offset, NULL, FILE_BEGIN ) == 0 ) {
                uint32_t last_error = ::GetLastError();
                throw last_error;
            }
            /* Устанавливает физический размер файла для указанного файла
             * в текущую позицию указателя файла. */
            /* Если функция выполнена успешно, возвращаемое значение отличное от нуля.
             * Если функция не работает, возвращаемое значение равно нулю (0).
             * Чтобы получить расширенную информацию об ошибке, вызовите GetLastError. */
            if ( ::SetEndOfFile( m_file ) == 0 ) {
                uint32_t last_error = ::GetLastError();
                throw last_error;
            }
            m_file_size.QuadPart = m_offset.QuadPart;
        }
#       else
        if ( m_offset.QuadPart < m_file_size.QuadPart ) {
            /* При успешном выполнеии lseek возвращает получившееся в результате смещение
             * в батах от начала файла. В противном случае, возвращается значение
             * (off_t)-1 и errno показывает ошибку. */
            int result = ::lseek( m_file, m_offset.QuadPart, SEEK_SET );
            if ( result == INVALID_HANDLE_VALUE ) {
                uint32_t last_error = errno;
                throw last_error;
            }

            /* Устанавливает физический размер файла для указанного файла
             * в текущую позицию указателя файла. */
            /* При успешной работе функции возвращаемое значение равно нулю.
             * При ошибке возвращается -1, а переменной errno присваивается номер ошибки. */
            result = ::ftruncate( m_file, m_offset.QuadPart );
            if ( result == INVALID_HANDLE_VALUE ) {
                uint32_t last_error = errno;
                throw last_error;
            }
            m_file_size.QuadPart = m_offset.QuadPart;
        }
#       endif  // defined(OS_WIN)

    }
    catch( uint32_t error ) {
        cout <<"an error number \""<< error <<"\" is generated in the method shrink_to_fit" <<endl;
    }
}   //  shrink_to_fit()

///////////////////////////////////////////////////////////////////////////////
// конвентирует wchar_t utf16 в string utf8 используя std::wstring_convert, std::codecvt.
string CFileMap::wchar_string ( const wchar_t *pwstr, size_t length /*= 0*/ )
{
    if ( !pwstr )
      return string();
    if ( !length )
        length = wcslen( pwstr );
    const wchar_t *first = pwstr;
    const wchar_t *last  = pwstr + length;
    wstring_convert<codecvt_utf8_utf16<wchar_t>, wchar_t> converter;
    return string( converter.to_bytes( first, last ) );
}   //  wchar_string ( const wchar_t *pwstr, size_t length /*= 0*/ )

///////////////////////////////////////////////////////////////////////////////
// конвентирует string utf8 в wstring utf16 используя std::wstring_convert, std::codecvt.
std::wstring CFileMap::char_wstring( const char *pstr, size_t length /*= 0*/ )
{
    if ( !pstr )
        return std::wstring();
    if ( !length )
        length = strlen( pstr );
    const char *first = pstr;
    const char *last  = pstr + length;
    std::wstring_convert<std::codecvt_utf8<wchar_t>, wchar_t> converter;
#if defined(OS_WIN) && defined(__GNUC__)
    /* bug MinGW 6.3 */
    std::wstring wstr = converter.from_bytes( first, last );
    for(std::size_t index=0; index < wstr.length(); index++) {
        int lb = LOBYTE(wstr[index]);
        int hb = HIBYTE(wstr[index]);
        if ( lb ) {
            wstr[index] = 0;
            wstr[index] = wstr[index] | hb;
            wstr[index] = wstr[index] | (lb<<8);
        } else {
            wstr[index] = wstr[index] >> 8;
        }
    }
    return std::wstring( wstr.begin(), wstr.end() );
#else
    return std::wstring( converter.from_bytes( first, last ) );
#endif  // defined(Q_OS_...)
}
