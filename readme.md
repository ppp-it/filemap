The projection of file in memory
================================

The object supports large files (>4Gb ).  
The functionality allows you to project the file partially.  
Text files can be read line by line.  
The object has its own interface for writing and reading to / from a file.  
In inheritance, the child has access to the projection address, and can  
operate directly with a pointer. CFileMap in inheritance provides an  
interface to control the boundaries of the current block and/or file.  
Inheritance allows you to point directly to data in a socket, eliminating  
additional read/write operations and unnecessary buffers.

an example of using inheritance:
--------------------------------
```C++
   /*
    1. object initialization  */
	
   if ( file_size > m_limit_memory )   
       set_limit_memory( m_limit_memory );	   
   else   
       set_limit_memory( 0 );
	   
   set_file_path( file_path );   
   set_file_size( file_size );   
   last_error = open_file_map( CFileMap::mode::read );
   
   ...
   
   /*   
    2. preparing and reading data from a file in the inherited class 	
	(when sending over the network) check the size of the next block 	   
	to send in order not to go beyond the file boundaries */
	   
   if ( m_buff_size > (file_size - file_start) )   
       read_size = (file_size - file_start);	   
   else   
       read_size = m_buff_size;
	   
   /* file_size   - file size   
    * file_start  - current position in the file	
    * m_buff_size - block size for file reading	
    * read_size   - calculated values of the block to read  */
	
   ...
   
   read_size = min( read_size, get_max_copy() );   
   bytes_sent = send( get_map_address(), read_size );  // sending data of size read_size   
   /* bytes_sent - number of bytes sent */  
  
   /* equivalent to reading from a file - just increase the pointer    
    * to the number of bytes sent */	
   check_map_region( bytes_sent );
   
   ...
   
   /*   
    3. writing data from a socket to a file in an inherited class, 	
	the function receive() takes the size of a read buffer   */
	
   ...
   read_size = m_buff_size;
   
    /* after setting the position in the file, 
     * check the size of the next block to read in order not to go beyond the boundaries of the file */
    if (read_size > ( file_size - file_start ))
	read_size = ( file_size - file_start);

    /* check that the projection allows you to write the desired number of bytes */
    read_size = min( read_size, (uint64_t)get_max_copy() );
   
   // get data from socket   
   bytes_received = receive( get_map_address(), (size_t)read_size );  // read based on maximum buffer size 
   /* bytes_sent - number of bytes received */
   
   /* equivalent to writing to a file-just increase the pointer 
    * to the number of bytes received from the socket */
   check_map_region( bytes_received );
   
   ...
 ```  
