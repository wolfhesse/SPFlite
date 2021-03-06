// compile: cl SPFSUBMIT.CPP /EHsc 

#define   _CRT_SECURE_NO_DEPRECATE   1

//////////////////////////////////////////////////////////////////////////////////////////
// System headers...

#define VC_EXTRALEAN        // Exclude rarely-used stuff from Windows headers

#define _WIN32_WINNT    0x0410      // (Windows 98 or later)
#define _WIN32_IE       0x0500      // (Internet Explorer 5 or later)

#include <afx.h>
#include <afxwin.h>         // MFC core and standard components
#include <afxext.h>         // MFC extensions
#include <afxdtctl.h>       // MFC support for Internet Explorer 4 Common Controls
#ifndef _AFX_NO_AFXCMN_SUPPORT
#include <afxcmn.h>         // MFC support for Windows Common Controls
#endif // _AFX_NO_AFXCMN_SUPPORT

#include <afxtempl.h>       // MFC template classes
#include <afxpriv.h>        // (ATL/MFC String Conversion Macros, etc)
#include <winsock2.h>       // sockets
#include <iostream>         // streams

#define NUM_CHARS(x)        (sizeof(x)/sizeof(char))
#define NUM_BYTES(x)        (sizeof(x))

#define PROG_NAME           "SPFSUBMIT"
#define PROG_PREFIX         PROG_NAME ": "
#define DIAG_PREFIX         _T(PROG_NAME ": rc=") << retcode << _T(": ")

#define RETCODE_OK          0    /* Successfully submitted\n")                                            */
#define RETCODE_CONNECT     1    /* Cannot connect to host, bad socket address, or connection refused\n") */
#define RETCODE_TIMEOUT     2    /* Timeout value exceeded while trying to connect\n")                    */
#define RETCODE_TRANSMIT    3    /* Transmission error, or connection prematurely closed\n")              */
#define RETCODE_FILE        4    /* File not found or cannot be opened\n")                                */
#define RETCODE_FAILURE     5    /* Submit failed, missing arguments or reason unknown\n")                */

#define DEFAULT_TIMEOUT     "5"

/////////////////////////////////////////////////////////////////////////////

using namespace std;

int _tmain( int argc, TCHAR* argv[], TCHAR* envp[] )
{
    int retcode = RETCODE_OK;

#if 0
    // Initialize MFC and print and error on failure

    if (!AfxWinInit( ::GetModuleHandle( NULL ), NULL, ::GetCommandLine(), 0 ))
    {
        retcode = RETCODE_FAILURE;

        cout << DIAG_PREFIX 
             << _T("MFC initialization failure\n");

        return retcode;
    }
#endif

    CString strSPFSUBMIT;

    strSPFSUBMIT.GetEnvironmentVariable(PROG_NAME);

    if (argc < (strSPFSUBMIT.IsEmpty() ? 3 : 2))
    {
        retcode = RETCODE_OK;       // requesting help is not an error

        cout
            << _T("\n" )
            << _T(PROG_PREFIX)
            << _T("Submit file to a given host:port address\n")
            << _T("\n")
            << _T("Command Syntax:\n")
            << _T("\n")
            << _T("    " PROG_NAME " [-nnn] [host:port] file [file ...]\n")
            << _T("\n")

            << _T("    -nnn        Timeout value (1-999) in seconds; " 
                  "default is " DEFAULT_TIMEOUT " seconds\n")

            << _T("    host:port   Target address to submit to; if not specified, the value\n")
            << _T("                of " PROG_NAME " environment variable is used, if defined\n")
            << _T("    file        File being submitted\n")
            << _T("\n")
            << _T("Examples:\n")
            << _T("\n")
            << _T("    " PROG_NAME " localhost:3505 job111.txt job222.txt\n")
            << _T("    set " PROG_NAME "=localhost:2501\n")
            << _T("    " PROG_NAME " job333.txt job444.txt\n")
            << _T("    " PROG_NAME " myhost:3505 job555.txt\n")
            << _T("    " PROG_NAME " 192.168.0.1:3505 job6.jcl job7.jcl\n")
            << _T("    " PROG_NAME " -2 127.0.0.1:3505 C:\\mypath\\myjob.jcl\n")
            << _T("\n")
            << _T("Return code values:\n")
            << _T("\n")
            << _T("    0    Successfully submitted\n")
            << _T("    1    Cannot connect to host, bad socket address, or connection refused\n")
            << _T("    2    Timeout value exceeded while trying to connect\n")
            << _T("    3    Transmission error, or connection prematurely closed\n")
            << _T("    4    File not found or cannot be opened\n")
            << _T("    5    Submit failed, missing or invalid arguments, or unexpected failure\n")
            << _T("\n")
            << _T(PROG_NAME " messages are written on stdout and can be redirected\n")
            << _T(PROG_NAME " with no parameters displays this help\n")
            ;

        return retcode;
    }

    // Gather arguments

    CString strTimeout, strHostPort;
    CArray < CString, LPCTSTR >  arFileNames;
    int i = 1, k, colon;

    if (argv[i][0] == _T('-'))
    {
        strTimeout = argv[i++];
        strTimeout = strTimeout.Mid(1);
    }

    strHostPort = argv[i++];

    if ((colon = strHostPort.Find(_T(':'))) < 1 && !strSPFSUBMIT.IsEmpty())
    {
        arFileNames.Add( strHostPort );
        strHostPort = strSPFSUBMIT;
    }

    while (i < argc)
    {
        arFileNames.Add( argv[i++] );
    }

    // Validate arguments

    if (strTimeout.IsEmpty()) strTimeout = _T(DEFAULT_TIMEOUT);

    int nTimeout = _ttoi( strTimeout );

    if (nTimeout < 1 || nTimeout > 999)
    {
        retcode = RETCODE_FAILURE;

        cout << DIAG_PREFIX
             << _T("Invalid timeout value (1-999)\n");

        return retcode;
    }

    if ((colon = strHostPort.Find(_T(':'))) < 1 || colon > (strHostPort.GetLength() - 1))
    {
        retcode = RETCODE_CONNECT;

        cout << DIAG_PREFIX
             << _T("Invalid socket address '") 
             << (LPCTSTR) strHostPort 
             << _T("'\n");

        return retcode;
    }

    CString  strHost  = strHostPort.Left( colon );
    CString  strPort  = strHostPort.Mid(  colon + 1 );

    int nPort = _ttoi( strPort );

    if (nPort < 1 || nPort > 65535)
    {
        retcode = RETCODE_CONNECT;

        cout << DIAG_PREFIX 
             << _T("Invalid socket address '") 
             << (LPCTSTR) strHostPort 
             << _T("'\n");

        return retcode;
    }

    CArray < CStdioFile*, CStdioFile*& >  arFiles;

    CStdioFile*     pFile;
    CFileException  fileException;
    TCHAR           szError[256];
    CString         strFileName;
    bool            bIsAscii = true;

    // Open all the specified files...

    for (i=0, k=arFileNames.GetSize(); i < k; i++)
    {
        strFileName = arFileNames[i];

        if (!i) // (do only for first file)
        {
            CFile f;    // (regular file; NOT stdio text)

            // Open in binary mode

            if (f.Open
            (
                strFileName,
                CFile::modeRead | CFile::shareDenyWrite | CFile::typeBinary,
                &fileException
            ))
            {
                // Read first 160 bytes and try to determine
                // whether file is actually ascii or ebcdic.

                unsigned char buf[ 160 ];

                UINT len = f.Read( buf, 160 );

                for (UINT i=0; i < len && buf[i] != _T('\x1A'); i++)
                {
                    if ((buf[i] < 0x20 || buf[i] > 0x7F)
                        && buf[i] != _T('\r') && buf[i] != _T('\n') && buf[i] != _T('\t'))
                    {
                        bIsAscii = false;

                        cout << _T(PROG_PREFIX) 
                             << _T("File encoded in EBCDIC\n");

                        break;
                    }
                }
            }
        }

        arFiles.Add( pFile = new CStdioFile );

        if (!pFile->Open
        (
            strFileName,
            0
            | CFile::modeRead
            | CFile::shareDenyWrite
            | (bIsAscii ? CFile::typeText : CFile::typeBinary)
            ,
            &fileException
        ))
        {
            szError[0] = 0;
            VERIFY( fileException.GetErrorMessage( szError, NUM_CHARS( szError )));

            retcode = RETCODE_FILE;

            cout << DIAG_PREFIX
                 << _T("Unable to open '") 
                 << (LPCTSTR) strFileName ;

            if (szError[0])
            {
                cout << _T("': ") 
                     << szError;
            }

            cout << _T("\n");

            return retcode;
        }
    }

    // Initialize Winsock

    if (retcode == RETCODE_OK)
    {
        WORD wVersionRequested;
        WSADATA wsaData;

        wVersionRequested = MAKEWORD(1,1);

        if (WSAStartup( wVersionRequested, &wsaData ) != 0)
        {
            retcode = RETCODE_CONNECT;

            cout << DIAG_PREFIX
                 << _T("Unable to initialize Winsock\n");

            return retcode;
        }
    }

    // Connect to the specified Hercules reader

    SOCKET rdr_socket = INVALID_SOCKET;
    u_long blockmode; // (to enable/disable non-blocking i/o)

    if (retcode == RETCODE_OK)
    {
        // (create the socket)

        rdr_socket = socket( AF_INET, SOCK_STREAM, IPPROTO_TCP );

        if (rdr_socket == INVALID_SOCKET)
        {
            retcode = RETCODE_CONNECT;

            cout << DIAG_PREFIX
                 << _T("Unable to create socket\n");
        }
        else
        {
            // (temporarily set it to non-blocking mode
            // for the duration of the connect so we can
            // specify a timeout value in the select)

            blockmode = 1;  // (enable non-blocking mode)

            if (ioctlsocket( rdr_socket, FIONBIO, &blockmode ) == SOCKET_ERROR)
            {
                retcode = RETCODE_CONNECT;

                cout << DIAG_PREFIX
                     << _T("ioctlsocket(FIONBIO=1) failed\n");
            }
            else
            {
                sockaddr_in sin; memset( &sin, 0, NUM_BYTES( sin ));

                sin.sin_family = AF_INET;
                sin.sin_port = htons( nPort );

                // (resolve the ip-number or hostname)

                if (_istdigit( strHost[0] ))
                {
                    sin.sin_addr.s_addr = inet_addr( strHost );
                }
                else
                {
                    LPHOSTENT he;

                    if (!(he = gethostbyname( strHost )))
                    {
                        retcode = RETCODE_CONNECT;

                        cout << DIAG_PREFIX
                             << _T("Invalid socket address '") 
                             << (LPCTSTR) strHostPort 
                             << _T("'\n");
                    }
                    else
                    {
                        sin.sin_addr.s_addr = *((u_long*)(he->h_addr));
                    }
                }

                if (retcode == RETCODE_OK)
                {
                    // (connect to the Hercules reader)

                    if (connect( rdr_socket, (struct sockaddr*) &sin,NUM_BYTES( sin )) == SOCKET_ERROR)
                    {
                        if (WSAGetLastError() != WSAEWOULDBLOCK)
                        {
                            retcode = RETCODE_CONNECT;

                            cout << DIAG_PREFIX
                                 << _T("Error connecting to '") 
                                 << (LPCTSTR) strHostPort 
                                 << _T("'\n");
                        }
                        else
                        {
                            // (wait for connect to complete)

                            fd_set writefds;

                            FD_ZERO( &writefds );
                            FD_SET( rdr_socket, &writefds );

                            struct timeval tv;
                            tv.tv_sec = nTimeout;
                            tv.tv_usec = 0;

                            if (select( 1, NULL, &writefds, NULL, &tv ) == SOCKET_ERROR)
                            {
                                if (WSAGetLastError() == WSAETIMEDOUT)
                                {
                                    retcode = RETCODE_TIMEOUT;

                                    cout << DIAG_PREFIX
                                         << _T("Timeout connecting to '") 
                                         << (LPCTSTR) strHostPort 
                                         << _T("'\n");
                                }
                                else
                                {
                                    retcode = RETCODE_CONNECT;

                                    cout << DIAG_PREFIX
                                         << _T("Error connecting to '") 
                                         << (LPCTSTR) strHostPort 
                                         << _T("'\n");
                                }
                            }
                        }
                    }
                }
            }
        }
    }

    // Reset the socket back to blocking mode

    if (retcode == RETCODE_OK)
    {
        blockmode = 0;  // (disable non-blocking mode)

        if (ioctlsocket( rdr_socket, FIONBIO, &blockmode ) == SOCKET_ERROR)
        {
            retcode = RETCODE_CONNECT;

            cout << DIAG_PREFIX
                 << _T("ioctlsocket(FIONBIO=0) failed\n");
        }
    }

    // Read the files in the order specified
    // and transmit them to the Hercules reader.

    if (retcode == RETCODE_OK)
    {
        CString strCard;
        int nLinesSubmitted;

        for (i=0, k=arFiles.GetSize(); i < k; i++)
        {
            pFile = arFiles[i];
            strFileName = arFileNames[i];
            nLinesSubmitted = 0;

            // The following may appear to be inefficient since we're doing
            // one I/O per card image, but because stdio files are buffered
            // and the socket functions are nagled, so there should not be
            // any appreciable loss in efficiency or speed.

            if (bIsAscii)
            {
                while (pFile->ReadString( strCard ))
                {
                    strCard += _T('\n');   // (was removed by ReadString)

                    if (send( rdr_socket, strCard, strCard.GetLength(), 0 ) == SOCKET_ERROR)
                    {
                        retcode = RETCODE_TRANSMIT;

                        cout << DIAG_PREFIX
                             << _T("Error transmitting '") 
                             << (LPCTSTR) strFileName 
                             << _T("'\n");
                        i = k;  // (force exit of outer loop too)
                        break;
                    }

                    nLinesSubmitted++;
                }
            }
            else
            {
                char buf[80];
                int len;

                while ((len = pFile->Read( buf, 80 )) > 0)
                {
                    if (send( rdr_socket, buf, len, 0 ) == SOCKET_ERROR)
                    {
                        retcode = RETCODE_TRANSMIT;

                        cout << DIAG_PREFIX
                             << _T("Error transmitting '") 
                             << (LPCTSTR) strFileName 
                             << _T("'\n");
                        i = k;  // (force exit of outer loop too)
                        break;
                    }

                    nLinesSubmitted++;
                }
            }

            TCHAR * plural;

            if (nLinesSubmitted == 0)
            {
                cout << DIAG_PREFIX
                     << _T("'") 
                     << (LPCTSTR) strFileName 
                     << _T("' not submitted") 
                     << _T("\n");
            }
            else
            {
                if (nLinesSubmitted == 1)
                {
                    plural = _T(""); 
                }

                else
                {
                    plural = _T("s");
                }

                cout << _T(PROG_PREFIX)
                     << _T("'") 
                     << (LPCTSTR) strFileName 
                     << _T("' submitted, ") 
                     << nLinesSubmitted 
                     << _T(" line") 
                     << plural
                     << _T("\n");
            }
        }
    }

    // Close all of the files...

    for (i=0, k=arFiles.GetSize(); i < k; i++)
    {
        delete arFiles[i];
    }

    if (rdr_socket != INVALID_SOCKET)
    {
        shutdown( rdr_socket, SD_BOTH );
        closesocket( rdr_socket );
    }

    WSACleanup();

#ifdef _DEBUG
    _tcout << _T(PROG_PREFIX) 
           << _T("\n(press any key to end program)\n");
    _gettc( stdin );
#endif
    return retcode;
}

