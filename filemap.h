/*!
 * \dir ./filemap
 * \brief проекция файла в память
 * \details объект поддерживает большие файлы ( >4Gb )\n
 * функционал позволяет проецировать файл частично, не целиком\n
 * текстовые файлы может считывать построчно\n
 * Объект имеет свой интерйфейс для записи и чтения в/из файла.\n
 * При наследовании потомок имеет доступ к адресу проекции, и может напрямую\n
 * оперировать указателем, при этом предоставляется интерфейс контролировать\n
 * границы текущего блока и файла.\n
 * Наследование позволяет напрямую отдавать указатель на данные в сокет,\n
 * исключая дополнительные операции чтения/записи и использование лишних\n
 * буферов.\n
 *
 * \file filemap.h
 * \brief определение класса проекция файла в память
 *
 *  объект поддерживает большие файлы ( >4Gb )\n
 *  функционал позволяет проецировать файл частично, не целиком\n
 *  текстовые файлы может считывать построчно\n
 *  Объект имеет свой интерйфейс для записи и чтения в/из файла.\n
 *  При наследовании потомок имеет доступ к адресу проекции, и может напрямую\n
 *  оперировать указателем, при этом предоставляется интерфейс контролировать\n
 *  границы текущего блока и файла.\n
 *  Наследование позволяет напрямую отдавать указатель на данные в сокет,\n
 *  исключая дополнительные операции чтения/записи и использование лишних\n
 *  буферов.\n
 *
 * Copyright (C) 2018 Pochepko PP.
 * Contact: ppp.it@hotmail.com
 *
 * This file is part of software written by Pochepko PP.
 *
 * This software is provided 'as-is', without any express or implied\n
 * warranty. In no event will the authors be held liable for any damages\n
 * arising from the use of this software.\n
 *
 * Permission is granted to anyone to use this software for any purpose,\n
 * including commercial applications, and to alter it and redistribute it\n
 * freely, subject to the following restrictions:\n
 *
 *    1. The origin of this software must not be misrepresented; you must not\n
 *    claim that you wrote the original software. If you use this software\n
 *    in a product, an acknowledgment in the product documentation would be\n
 *    appreciated but is not required.\n
 *
 *    2. Altered source versions must be plainly marked as such, and must not be\n
 *    misrepresented as being the original software.\n
 *    3. This notice may not be removed or altered from any source\n
 *    distribution.\n
 *
 */
//-!-!-!-!-!-!-!-!-!-!-!-!-!-!-!-!-!-!-!-!-!-!-!-!-!-!-!-!-!-!-!-!-!-!-!-!-!-!-!-!-!-!-!-!-!-!-!-!-!-!


#ifndef FILEMAPILEMAP_H
#define FILEMAPILEMAP_H

#if !defined(__STDC_WANT_LIB_EXT1__)
#   define __STDC_WANT_LIB_EXT1__ 1
#endif
#include <string.h>
#include <string>

#if ( defined(WIN64) || defined(_WIN64) || defined(__WIN64__) )
#  define OS_WIN32
#  define OS_WIN64
#  define OS_WIN
#elif ( defined(WIN32) || defined(_WIN32) || defined(__WIN32__) || defined(__NT__) )
#  define OS_WIN32
#  define OS_WIN
#endif

#if defined(__linux__) || defined(__linux)
#  define OS_LINUX
#endif

#if defined(OS_WIN)
#  include <Windows.h>
#else
#  include <sys/mman.h>     //  mmap
#  include <sys/types.h>    //  open
#  include <sys/stat.h>     //  open
#  include <fcntl.h>        //  open
#  include <unistd.h>       //  getpagesize, mmap
#  if !defined(INVALID_HANDLE_VALUE)
#      define INVALID_HANDLE_VALUE (int)-1
#  endif
   typedef int HANDLE;

  typedef union _LARGE_INTEGER {
    struct {
      uint32_t LowPart;
      uint32_t  HighPart;
    };
    struct {
      uint32_t LowPart;
      uint32_t  HighPart;
    } u;
    uint64_t QuadPart;
  } LARGE_INTEGER, *PLARGE_INTEGER;
#endif  // defined(OS_WIN)



//--------------------------------------------------------------------------------------------------//




///////////////////////////////////////////////////////////////////////////////
/// \brief The CFileMap class - проекция файла в память
///
/// объект поддерживает большие файлы ( >4Gb )\n
/// функционал позволяет проецировать файл частично, не целиком\n
/// текстовые файлы может считывать построчно\n
/// Объект имеет свой интерйфейс для записи и чтения в/из файла.\n
/// При наследовании потомок имеет доступ к адресу проекции, и может напрямую\n
/// оперировать указателем. CFileMap предоставляется интерфейс контролировать\n
/// границы текущего блока и файла.\n
/// Наследование позволяет напрямую отдавать указатель на данные в сокет,\n
/// исключая дополнительные операции чтения/записи и использование лишних\n
/// буферов.\n
///
/// \code
/// /*
/// 1. инициализация и создание объекта  */
/// if ( file_size > m_limit_memory )
///     set_limit_memory( m_limit_memory );
/// else
///     set_limit_memory( 0 );
/// set_file_path( file_path );
/// set_file_size( file_size );
/// last_error = open_file_map( CFileMap::mode::read );
/// ...
/// /*
/// 2. подготовка и чтение данных из файла в наследуемом классе (при отправке по сети)   */
/// /* проверм размер следующего блока для отправки чтобы не выйти за границы файла */
/// if ( m_buff_size > (file_size - file_start) )
///     read_size = (file_size - file_start);
/// else
///     read_size = m_buff_size;
/// /* file_size   - размер файла
///  * file_start  - текущая позиция в файле
///  * m_buff_size - размер блока для поблочного чтения файла
///  * read_size   - расчитанное значения блока для считывания  */
/// ...
/// read_size = min( read_size, get_max_copy() );
/// bytes_sent = send( get_map_address(), read_size );  // отправка данных размером read_size
/// /* bytes_sent - количество отправленных байт */
///
/// /* равносильно чтению из файла - просто увеличим указатель на
///  * количество отправленных байт */
/// check_map_region( bytes_sent );
/// ...
/// /*
/// 3. запись данных из сокета в файл в наследуемом классе,
///    функция receive() принимает размер буфера для чтения   */
/// ...
/// // получим данные из сокета
/// bytes_received = receive( get_max_copy() );  // чтение с учетом максимального размера буфера
/// check_map_region( bytes_received )
/// ...
/// \endcode
///
class CFileMap
{

    ///////////////////////////////////////////////////////////////////////////////
    /// \brief режим доступа к файлу, страницам памяти и к проекции файла
#define NO_FLAG   0        /* заглушка для несуществующих флагов в другой OS */
    // Режим совместного доступа к файлу
#define SHARE_DENIED       0                                         /* монопольный доступ к файлу */

#if defined(OS_WIN)

    // Режим доступа к файлу
//GENERIC_READ            /* 0x80000000 */       /* файл открыт для чтения */
//GENERIC_WRITE           /* 0x40000000 */       /* файл открыт для записи */

    // Режим совместного доступа к файлу
//0                                          /* монопольный доступ к файлу */
//FILE_SHARE_READ         /* 0x00000001 */   /* доступен другим для чтения */
//FILE_SHARE_WRITE        /* 0x00000002 */   /* дотупен другим для записи  */

    // Обработка существующего и/или не существующего файла
//CREATE_NEW              /* 1 */  /* Создает новый файл.                  */
//CREATE_ALWAYS           /* 2 */  /* Создает новый даже если существует   */
//OPEN_EXISTING           /* 3 */  /* Открывает только существующий файл.  */
//OPEN_ALWAYS             /* 4 */  /* Открывает файл или создает новый     */
//TRUNCATE_EXISTING       /* 5 */  /* Открывает файл и обрезает его в ноль */

    // Флаги атрибутов файла
//FILE_ATTRIBUTE_READONLY /* 0x00000001 */  /* доступен только для чтения  */
//FILE_ATTRIBUTE_HIDDEN   /* 0x00000002 */  /* Файл скрытый.               */
//FILE_ATTRIBUTE_SYSTEM   /* 0x00000004 */  /* Файл является частью OS.    */
//FILE_ATTRIBUTE_ARCHIVE  /* 0x00000020 */  /* Файл готов к архивированию. */
//FILE_ATTRIBUTE_NORMAL   /* 0x00000080 */  /* У файла нет атрибутов       */
//FILE_ATTRIBUTE_TEMPORARY/* 0x00000100 */  /* Временный файл              */
//FILE_ATTRIBUTE_OFFLINE  /* 0x00001000 */  /* Данные доступны не сразу.   */
//FILE_ATTRIBUTE_ENCRYPTED/* 0x00004000 */  /* Файл или каталог шифруются  */

    // Режим защиты страниц памяти
//PAGE_READONLY           /* 0x02 */
//PAGE_READWRITE          /* 0x04 */
//PAGE_WRITECOPY          /* 0x08 */
//PAGE_EXECUTE_READ       /* 0x20 */
//PAGE_EXECUTE_READWRITE  /* 0x40 */
//PAGE_EXECUTE_WRITECOPY  /* 0x80 */

    // Режим доступа к объекту "проекция файла"
//FILE_MAP_READ           /* 0x0004 */
//FILE_MAP_WRITE          /* 0x0002 */
//FILE_MAP_ALL_ACCESS     /* 0x000F0000 0x0001 0x0002 0x0004 0x0008 0x0010 */
//FILE_MAP_COPY           /* 0x1 */
//FILE_MAP_EXECUTE        /* 0x0020 */

#else

//    // Режим доступа к файлу
//O_RDONLY          /* 00000000 */    /* файл открыт для чтения               */
//O_WRONLY          /* 00000001 */    /* файл открыт для записи               */
//O_RDWR            /* 00000002 */    /* файл открыт для чтения и записи      */

//    // Обработка существующего и/или не существующего файла
//O_CREAT           /* 00000100 */    /* Создает новый файл.                   */
//O_TRUNC           /* 00001000 */    /* Открывает файл и обрезает его в ноль  */
//O_APPEND          /* 00002000 */    /* Файл открывается в режиме добавления. */

//    // Режим защиты страниц памяти
//PROT_READ
//PROT_WRITE
//PROT_EXEC

//    // Режим доступа к объекту "проекция файла"
//MAP_FIXED
//MAP_SHARED
//MAP_PRIVATE
#endif  // defined(OS_WIN)


public:
    ///////////////////////////////////////////////////////////////////////////////
    /// \brief The mem_data union - объединение указателя и числового типа\n
    ///  указатель - для доступа к адресу памяти\n
    ///  числовой  - для математических операций над указателем
    ///
    union mem_data  {
        struct {
          uint32_t LowPart;
          uint32_t HighPart;
        };
        struct {
          uint32_t LowPart;
          uint32_t HighPart;
        } u;
        void*    map_ptr;  // адрес, куда отображается файл, изменяемый при обработке
        uint64_t map_mth;  // адрес, куда отображается файл, для математических операций
    };
    typedef mem_data *pmem_data;

public:
    ///////////////////////////////////////////////////////////////////////////////
    /// \brief перечисление предопределенных режимов обработки объекта
    ///
    enum class mode : uint64_t
    {
        /*! сам файл, страницы памяти и проекция файла с доступом только чтение.
         *  Другим потокам/процесам разрешен доступ на чтение.\n
         *  Файл обязательно должен существовать. */
        read,

        /*! Доступ на запись для файла, страниц памяти и объекта проекции.
         *  Другим потокам/процесам запрещен доступ к файлу.\n
         *  если файл не существует - то создается,\n
         *  если существует - то размер усекается до нуля. */
        write,

        /*! Доступ на запись для файла, страниц памяти и объекта проекции.
         *  Другим потокам/процесам разрешен доступ на чтение.\n
         *  Файл обязательно должен существовать. */
        append
    };

public:
    ///////////////////////////////////////////////////////////////////////////////
    /// \brief конструктор
    /// \param limit_map_memory - размер ограниения для единовремменого отражения\n
    ///  файла в память, если равен нулю - то отражается сразу весь файл целиком.
    ///
    CFileMap( uint64_t limit_map_memory = 0 );

public:
    ///////////////////////////////////////////////////////////////////////////////
    /// \brief деструктор
    ~CFileMap();

public:
    ///////////////////////////////////////////////////////////////////////////////
    /// \brief  открыть файл для последющего отображения используя флаги
    /// \param  offset - смещение байт от начала файла
    /// \param  md_fl - Флаги режима доступа к файлу
    /// \param  md_sh - Флаги для режима совместного доступа к файлу
    /// \param  md_op - Флаги для обработки существующего и/или не существующего файла
    /// \param  md_at - Флаги атрибутов файла
    /// \param  md_pp - Флаги режима защиты страниц памяти
    /// \param  md_mm - Флаги режима доступа к объекту "проекция файла"
    /// \return ноль - выполнено успешно, иначе номер ошибки
    ///
#if defined(OS_WIN)
    uint64_t open_file_map ( uint32_t md_fl = GENERIC_READ,                /* mode file access      */
                             uint32_t md_sh = SHARE_DENIED,                /* mode share access     */
                             uint32_t md_op = OPEN_EXISTING,               /* mode file open/create */
                             uint32_t md_at = FILE_ATTRIBUTE_NORMAL,       /* flag file attribute   */
                             uint32_t md_pp = PAGE_READONLY,               /* mode page protect     */
                             uint32_t md_mm = FILE_MAP_READ,               /* mode map access       */
                             uint64_t offset = 0 );                        /* смещение от начала    */
#else
    uint64_t open_file_map ( uint32_t md_fl = O_RDONLY,                    /* mode file access      */
                             uint32_t md_op = NO_FLAG,                     /* mode file open/create */
                             uint32_t md_pp = PROT_READ,                   /* mode page protect     */
                             uint32_t md_mm = MAP_PRIVATE,                 /* mode map access       */
                             uint64_t offset = 0,                          /* смещение от начала    */
                             mode_t mode = S_IRWXU|S_IRWXG|S_IROTH );      /* задает права доступа  */
#endif  // defined(OS_WIN)

public:
    ///////////////////////////////////////////////////////////////////////////////
    /// \brief  открыть файл для последющего отображения используя предопределенный режим
    /// \param  offset - смещение байт от начала файла
    /// \param  md - режим обработки файла и проекции ( read, write, append )
    /// \return ноль - выполнено успешно, иначе номер ошибки
    /// @see CFileMap::mode
    ///
    uint64_t open_file_map ( mode md, uint64_t offset = 0 );

public:
    ///////////////////////////////////////////////////////////////////////////////
    bool is_open () {
        return ( m_file != INVALID_HANDLE_VALUE && m_address.map_mth > 0 );
    }

public:
    ///////////////////////////////////////////////////////////////////////////////
    /// \brief закрывает объект
    /// \param b_shrink_to_fit - флаг (подогнать газмер файла под размер данных)
    ///  b_shrink_to_fit = true - подогнать газмер файла под размер данных
    ///  b_shrink_to_fit = false - оставить размер как есть (=m_file_size)\n
    ///    при этом данные могут быть записаны не до конца файла
    ///
    void close_file_map( bool b_shrink_to_fit = false );

public:
    ///////////////////////////////////////////////////////////////////////////////
    /// \brief установить полное имя файла
    /// \param file_path - полное имя файла
    ///
    void set_file_path( const wchar_t *file_path ) {
        m_file_path = file_path;
    }

public:
    ///////////////////////////////////////////////////////////////////////////////
    /// \brief установить полное имя файла
    /// \param file_path - полное имя файла
    ///
    void set_file_path( const char *file_path ) {
        m_file_path = char_wstring( file_path );
    }

public:
    ///////////////////////////////////////////////////////////////////////////////
    /// \brief установить размер файла
    /// \param file_size - размер файла
    ///
    void set_file_size( uint64_t file_size ) {
        m_file_size.QuadPart = file_size;
    }

public:
    ///////////////////////////////////////////////////////////////////////////////
    /// \brief установить смещение от начала файла
    /// \param file_offset - смещение от начала файла
    /// @warning смещение можно установить до создания проекуии (открытия файла)
    void set_file_offset( uint64_t file_offset ) {
        if ( m_ptr_file == nullptr ) {
            m_offset.QuadPart = file_offset;
        }
    }

public:
    ///////////////////////////////////////////////////////////////////////////////
    /// \brief установить размер ограниения для единовремменого отражения файла\n
    ///       в память, установить можно только один раз и только до открытия файла
    /// \param limit_memory - размер ограниения для единовремменого отражения файла в память
    ///
    void set_limit_memory( uint64_t limit_map_memory ) {
        if ( m_ptr_file == 0 && m_limit_memory == 0 ) {
            m_limit_memory = memory_allocation_granularity( limit_map_memory );
        }
    }

public:
    ///////////////////////////////////////////////////////////////////////////////
    /// \brief end of file
    /// \return true- end of file
    ///
    bool eof() {
        return ( m_offset.QuadPart >= m_file_size.QuadPart );
    }

public:
    ///////////////////////////////////////////////////////////////////////////////
    /// \brief записать данные в файл
    /// \param str - строка для записи
    /// \return количество записанных байт
    ///
    uint64_t write( std::string &str ) {
        return write( str.c_str(), str.length() );
    }

public:
    ///////////////////////////////////////////////////////////////////////////////
    /// \brief записать данные в файл
    /// \param str - строка для записи
    /// \param length - количство байт для записи
    /// \return количество записанных байт
    ///
    uint64_t write( const char *str, uint64_t length );

public:
    ///////////////////////////////////////////////////////////////////////////////
    /// \brief записать строку в файл
    ///  эта функция добавляет символ новой строки к данным\n
    /// и вызывает функцию write()
    /// \param str - строка для записи
    /// \return количество записанных байт
    /// @see write
    uint64_t write_line( std::string &str ) {
        str.append( m_new_line );
        return write( str.c_str(), str.length() );
    }

public:
    ///////////////////////////////////////////////////////////////////////////////
    /// \brief прочитать строку из файла
    /// \param str - буфер для записи строки из файла
    /// \return количество прочитанных байт
    ///
    uint64_t read_line( char *dest );

public:
    ///////////////////////////////////////////////////////////////////////////////
    /// \brief прочитать данные из файла
    /// \param str - буфер для записи данных из файла
    /// \param length - максимальное количество байт, которе может принять буфер
    /// \return количество прочитанных байт
    ///
    uint64_t read( char *dest, uint64_t length );

protected:
    ///////////////////////////////////////////////////////////////////////////////
    /// \brief передает указатель классу - наследнику\n
    /// реализация предоставляет возможность передавать\n
    /// указатель проекции в класс наследник, но при этом\n
    /// должны быть выполнены следующие условия:\n
    ///  1. изменение адреса должно выполнятся блоками,\n
    ///     кратными странице памяти\n
    ///  2. после каждого изменения адреса класс наследник\n
    ///     должен вызывать функцию check_map_region() и обновлять\n
    ///     адресс проекции, что бы не выйти за пределы региона\n
    /// \return адрес проекции
    /// \see check_map_region()
    inline void* get_map_address() {
        return m_address.map_ptr;
    }

protected:
    ///////////////////////////////////////////////////////////////////////////////
    /// \brief получить максимальное количество оставшихся байт из проекции
    /// \return
    ///
    inline uint32_t get_max_copy() const {
        return (uint32_t)m_max_copy;
    }

protected:
    ///////////////////////////////////////////////////////////////////////////////
    /// \brief проверка выделеного региона и проекция следующего
    /// \param data_size - количество прочитанных/записанных байт\n
    /// если адрес проекции был передан в класс наследник,\n
    /// то текущий класс теряет контроль на выделеным регионом\n
    /// что бы не потерять контроль и своевреммено проецировать\n
    /// следующий регион файла после каждой операции с адрессом\n
    /// наследник должен вызывать эту функцию
    /// \return адрес проекции
    void* check_map_region( uint64_t data_size );

//protected:
//    ///////////////////////////////////////////////////////////////////////////////
//    /// \brief проверка выделеного региона и проекция следующего
//    /// \param address - адрес проекции из класса наследника\n
//    ///  алгоритм такойже как и в функции check_map_region( uint64_t data_size )
//    /// \return адрес проекции
//    ///  \see check_map_region( uint64_t data_size )
//    void* check_map_region( void* address ) {
//        if ( address == 0 )
//            return address;
//        uint64_t length = ((uint64_t)address) - m_address.map_mth;
//        check_map_region( length );
//    }

protected:
    ///////////////////////////////////////////////////////////////////////////////
    /// \brief отразить в память следующую часть файла
    ///
    /// внутри вызывает map_region
    /// \return ноль - выполнено успешно, иначе номер ошибки
    /// @see map_region()
    uint64_t next_region();

protected:
    ///////////////////////////////////////////////////////////////////////////////
    /// \brief  выравнивание региона с учетом гранулярности страниц памяти в OS
    /// \param  region - размер для выравнивания кратности размеру страницы
    /// \return значение, кратное размеру страницы (увеличено в большую сторону)
    ///
    uint64_t memory_allocation_granularity( uint64_t region ) {
        return (region + m_page_size-1) & ~(m_page_size-1);
    }

protected:
    ///////////////////////////////////////////////////////////////////////////////
    /// \brief  отражает файл (часть файла) в память
    /// \param  offset - смещение байт от начала файла
    /// \param  size_region - размер отражаемых в память байтов
    /// \return ноль - выполнено успешно, иначе номер ошибки
    ///
    /// m_ptr_file равен адресу отражения - если успешно,\n
    /// m_ptr_file равен нулю в случае ошибки.
    ///
    uint64_t map_region ( uint64_t offset = 0, uint64_t size_region = 0 );

protected:
    ///////////////////////////////////////////////////////////////////////////////
    /// \brief  unmap_region - снимет отражение в памяти (освобождает память)
    /// \param  size_region  - размер памяти для синхронизации (*nix - и для освобождения)
    /// \param  sync         - флаг синхронизации памяти с файлом,\n
    ///                        true - синхронизировать сразу после закрытия\n
    ///                        false - система сама выполнит синхронизацию\n
    /// \return ноль - выполнено успешно, иначе номер ошибки
    ///
    uint64_t unmap_region( uint64_t size_region, bool sync = true );

protected:
    ///////////////////////////////////////////////////////////////////////////////
    /// \brief копирует length байт из участка памяти, на который указывает указатель\n
    /// src_ptr, в переменную-член m_address.map_ptr, при этом гарантируется, что\n
    /// что переполнения буфера не будет.
    /// \param src_ptr - указатель на адрес источника для копирования
    /// \param length  - количество байт, которые нужно скопировать
    /// \return количество записанных байт.
    ///
    uint64_t write2memory ( const void *src_ptr, uint64_t length );

protected:
    ///////////////////////////////////////////////////////////////////////////////
    /// \brief копирует length байт из переменной-члена m_address.map_ptr в участок\n
    ///  памяти, на который указывает указатель dest_ptr, при этом в dest_ptr\n
    /// должно быть достаточно места для копирования length байт.
    /// \param dest_ptr- указатель на адрес получателя для копирования
    /// \param length  - количество байт, которые нужно скопировать
    /// \return количество прочитанных байт.
    ///
    uint64_t read_from_memory ( void *dest_ptr, uint64_t length );

protected:
    ///////////////////////////////////////////////////////////////////////////////
    /// \brief get_page_size - Get the system allocation granularity.
    /// \return размер страницы памяти в OS
    ///
    uint64_t get_page_size ()  {
#       if defined(OS_WIN)
        // Get the system allocation granularity.
        SYSTEM_INFO SysInfo;
        ::GetSystemInfo( &SysInfo );
        return ( SysInfo.dwAllocationGranularity );
#       else
        /* ...Поэтому рекомендуется определять PAGE_SIZE не на стадии компиляции из
         * файла заголовка, а при выполнении программы с помощью данной функции ... */
        return ( ::getpagesize() );
#       endif  // defined(OS_WIN)
    }

protected:
    ///////////////////////////////////////////////////////////////////////////////
    /// \brief  конвентирует wstring utf16 в string utf8 используя std::wstring_convert, std::codecvt.
    /// \param  pwstr        wchar_t в формате utf16
    /// \return строка std::string (utf8)
    ///
    std::string wchar_string( const wchar_t *pwstr, uint64_t length = 0 );

protected:
    ///////////////////////////////////////////////////////////////////////////////
    /// \brief конвентирует string utf8 в wstring utf16 используя std::wstring_convert, std::codecvt.
    /// \param [in]     pstr     набор символов char (utf8)
    /// \return         строка std::wstring (utf16)
    ///
    std::wstring char_wstring( const char *pstr, uint64_t length = 0 );

private:
    ///////////////////////////////////////////////////////////////////////////////
    /// \brief подогнать газмер файла под размер данных
    ///
    void shrink_to_fit();

private:
    ///////////////////////////////////////////////////////////////////////////////
    /// \brief установить максимальное количество байт доступных для чтения/записи\n
    /// и обновляет смещения в файле m_offset и в текущем блоке m_offset_block.
    /// \param length - значение успешно записанных/прочитанных байт
    ///
    /// в режиме блочной проекции файла выполняется условие:\n
    ///     m_limit_memory = m_offset_block + m_max_copy\n
    /// в режиме, когда файл открыт целиком, выполняется условие:\n
    ///     m_file_size = m_offset.QuadPart + m_max_copy\n
    ///
    void set_max_copy( uint64_t length = 0 );



//--------------------------------------------------------------------------------------------------//




protected:
    ///////////////////////////////////////////////////////////////////////////////
    /// \brief размер файла
    ///
    /// в режиме блочной проекции файла выполняется условие:\n
    ///     m_limit_memory = m_offset_block + m_max_copy\n
    /// в режиме, когда файл открыт целиком, выполняется условие:\n
    ///     m_file_size = m_offset.QuadPart + m_max_copy
    ///
    LARGE_INTEGER m_file_size;

protected:
    ///////////////////////////////////////////////////////////////////////////////
    /// \brief смещение от начала файла
    ///
    /// устанавливеается автоматически в методах записи-чтения (вызовом set_max_copy),\n
    /// можно явно установить в методе создания проекции map_region,\n
    /// так же можно установить в методе set_file_offset\n
    /// но только до создания проекции.
    ///
    /// @see CFileMap::set_max_copy
    /// @see CFileMap::map_region
    /// @see CFileMap::set_file_offset
    ///
    LARGE_INTEGER m_offset;

protected:
    ///////////////////////////////////////////////////////////////////////////////
    /// \brief смещение от начала текущего блока проекции\n
    /// устанавливеается автоматически в методах записи-чтения.
    ///
    /// в режиме блочной проекции файла выполняется условие:\n
    ///     m_limit_memory = m_offset_block + m_max_copy\n
    /// в режиме, когда файл открыт целиком, выполняется условие:\n
    ///     m_file_size = m_offset.QuadPart + m_max_copy
    ///
    uint64_t m_offset_block;

protected:
    ///////////////////////////////////////////////////////////////////////////////
    /// \brief адрес, куда отображается файл\n
    /// (изменяемый в процессе обработки)
    /// @see CFileMap::map_region()
    /// @see CFileMap::unmap_region()
    /// @see CFileMap::write2memory())
    /// @see CFileMap::read_from_memory()
    ///
    mem_data m_address;

private:
    ///////////////////////////////////////////////////////////////////////////////
    /// \brief описатель файла
    ///
    HANDLE m_file;

private:
    ///////////////////////////////////////////////////////////////////////////////
    /// \brief режим доступа к проекции
    ///
    ///  передается параметром в функцию open_file_map\n
    ///  а используется в функции map_region
    /// @see CFileMap::open_file_map()
    /// @see CFileMap::map_region()
    DWORD m_map_mode;

#   if ( defined(OS_LINUX) || defined(OS_UNUX) )
private:
    ///////////////////////////////////////////////////////////////////////////////
    /// \brief режим доступа к страницам памяти
    ///  передается параметром в функцию open_file_map
    ///  а используется в функции map_region только в *nix системах.
    ///
    uint64_t m_page_protect;
#   endif  // defined(OS_WIN)

#   if defined(OS_WIN)
    ///////////////////////////////////////////////////////////////////////////////
    /// \brief описатель проекции файла
    ///
    HANDLE m_hFileMapping;
#   endif  // defined(OS_WIN)

private:
    ///////////////////////////////////////////////////////////////////////////////
    /// \brief полное имя файла
    ///
    std::wstring m_file_path;

private:
    ///////////////////////////////////////////////////////////////////////////////
    /// \brief m_page_size - размер страницы памяти в OS
    ///
    uint64_t m_page_size;

private:
    ///////////////////////////////////////////////////////////////////////////////
    /// \brief размер ограниения для единовремменого отражения файла в память\n
    ///  если равен нулю - то отражается сразу весь файл целиком.
    ///
    /// в режиме блочной проекции файла выполняется условие:\n
    ///     m_limit_memory = m_offset_block + m_max_copy\n
    /// в режиме, когда файл открыт целиком, выполняется условие:\n
    ///     m_file_size = m_offset.QuadPart + m_max_copy
    ///
    uint64_t m_limit_memory;

private:
    ///////////////////////////////////////////////////////////////////////////////
    /// \brief максимальное количество байт для копирования,\n
    /// определяется как разница между размером файла\n
    /// (m_file_size) и указателем на текущую позицию в файле\n
    /// (m_offset). А если используется режим блочного доступа\n
    /// то разница между размером блока (m_limit_memory) и\n
    /// указателем на текущую позицию в блоке (m_offset_block).
    ///
    /// в режиме блочной проекции файла выполняется условие:\n
    ///     m_limit_memory = m_offset_block + m_max_copy\n
    /// в режиме, когда файл открыт целиком, выполняется условие:\n
    ///     m_file_size = m_offset.QuadPart + m_max_copy
    ///
    uint64_t m_max_copy;

private:
    ///////////////////////////////////////////////////////////////////////////////
    /// \brief адрес, куда отображается файл\n
    /// (не изменяемый - для освобождения ресурса)
    /// @see CFileMap::map_region()
    /// @see CFileMap::unmap_region()
    ///
    void* m_ptr_file;

private:
    ///////////////////////////////////////////////////////////////////////////////
    /// \brief флаг синхронизации данных файла с дисковой системой
    /// если файл открыт в режиме чтения - то синхронизировать не нужно
    ///
    bool m_sync;

#if defined(OS_WIN)
private:
    ///////////////////////////////////////////////////////////////////////////////
    /// \brief флаг возврата коретки, при считывании строки в системе Windows\n
    /// символ переход на новую строку состоит из двух символов:\n
    /// воврат каретки (код 13)\n
    /// новая строка (код 10).\n
    /// так при открытии файла блоками ( m_limit_memory > 0 ) может получится так\n
    /// что символ перехода на новую строку разделяется между проекциями. в конец\n
    /// первой проекции попадает символ возврата, а в начало следующей проекции\n
    /// символ новой строки. Поэтому в методе read_line при считывании в блочном\n
    /// режиме перед переходном на новый регион проверяется последний символ\n
    /// текущего региона, и если он равен символу возврата - устанавливается\n
    /// этот флаг. после открытия нового региона проверяется состояние этого\n
    /// флага - и если флаг поднят (последним был симол возврата) то проверяется\n
    /// первый символ нового региона - после чего флаг сбрасывается, не зависимо\n
    /// от значения первого символа (если первый символ - новая строка)\n
    /// то считается что достигнут конец текущей считываемой строки и происходит\n
    /// возврат из метода read_line, иначе считывание продолжается до тех пор,\n
    /// пока не будет найден конец строки.\n
    ///
    bool m_return;
#endif  // defined(OS_WIN)

private:
    ///////////////////////////////////////////////////////////////////////////////
    /// \brief Определяемый платформой (подходящий) символ новой строки.
    /// Chr(13) & Chr(10) для Windows
    /// Chr(10)  для Linux (Unix)
#if defined(OS_WIN)
    char m_new_line[2];
#else
    char m_new_line[1];
#endif  // defined(OS_WIN)
};

#endif // FILEMAPILEMAP_H
