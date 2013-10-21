//
// SerialChannel_POSIX.cpp
//
// $Id: //poco/Main/DeviceIO/Serial/src/SerialChannel_POSIX.cpp#1 $
//
// Library: Serial
// Package: Serial
// Module:  SerialChannel
//
// Copyright (c) 2008, Applied Informatics Software Engineering GmbH.
// and Contributors.
//
// Permission is hereby granted, free of charge, to any person or organization
// obtaining a copy of the software and accompanying documentation covered by
// this license (the "Software") to use, reproduce, display, distribute,
// execute, and transmit the Software, and to prepare derivative works of the
// Software, and to permit third-parties to whom the Software is furnished to
// do so, all subject to the following:
// 
// The copyright notices in the Software and this entire statement, including
// the above license grant, this restriction and the following disclaimer,
// must be included in all copies of the Software, in whole or in part, and
// all derivative works of the Software, unless such copies or derivative
// works are solely in the form of machine-executable object code generated by
// a source language processor.
// 
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE, TITLE AND NON-INFRINGEMENT. IN NO EVENT
// SHALL THE COPYRIGHT HOLDERS OR ANYONE DISTRIBUTING THE SOFTWARE BE LIABLE
// FOR ANY DAMAGES OR OTHER LIABILITY, WHETHER IN CONTRACT, TORT OR OTHERWISE,
// ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
// DEALINGS IN THE SOFTWARE.
//


#include "Poco/DeviceIO/Serial/SerialChannel_POSIX.h"
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <termios.h>
#include <cstring>
#include "Poco/Exception.h"


using Poco::CreateFileException;
using Poco::IOException;


namespace Poco {
namespace DeviceIO {
namespace Serial {


SerialChannelImpl::SerialChannelImpl(SerialConfigImpl* pConfig): 
	_handle(0),
	_pConfig(pConfig, true)
{
    std::cout << _pConfig->name() << std::endl;
//	openImpl();
}


SerialChannelImpl::~SerialChannelImpl()
{
    std::cout << "Destroying SerialChannelImpl_POSIX " << std::endl;
//	closeImpl();
}


void SerialChannelImpl::initImpl()
{
    std::cout << "setting attr = " << _handle << std::endl;
//	tcsetattr(_handle, TCSANOW, &_pConfig->getTermios());
//    std::cout << ">" << _pConfig->getTermios().



    cfsetispeed(&_pConfig->getTermios(),B38400);
    cfsetospeed(&_pConfig->getTermios(),B38400);

    _pConfig->getTermios().c_cflag |= (CLOCAL | CREAD);
    _pConfig->getTermios().c_cflag &= ~PARENB;
    _pConfig->getTermios().c_cflag &= ~CSTOPB;
    _pConfig->getTermios().c_cflag &= ~CSIZE;
    _pConfig->getTermios().c_cflag |= CS8;
    tcsetattr(_handle,TCSANOW,&_pConfig->getTermios());

}


void SerialChannelImpl::openImpl()
{
//	initImpl();
    std::cout << "===== getting handle for >" << _pConfig->name().c_str() << "< handle=>" << _handle << std::endl;

    _handle = open(_pConfig->name().c_str(), O_RDWR | O_NOCTTY | O_NDELAY);

//    _handle = open(_pConfig->name().c_str(), O_RDWR | O_NOCTTY | O_NONBLOCK);

    std::cout << "===== handle ==" << _handle << std::endl;

	if (_handle == -1)
    {
        std::cout << "===== handling error ..." << std::endl;
        handleError(_pConfig->name());
    }
    initImpl();

}


void SerialChannelImpl::closeImpl()
{
	if (_handle) close(_handle);
}


int SerialChannelImpl::readImpl(char* pBuffer, int length)
{
	int readLen = 0;
	int readCount = 0;
	std::memset(pBuffer, 0, length);
	do
	{
		if ((readLen = read(_handle, pBuffer + readCount, length - readCount)) < 0) 
			handleError(_pConfig->name());
		else if (0 == readLen) break;

		poco_assert (readLen <= length - readCount);
		readCount += readLen;
	}while(readCount < length);

	return readCount;
}


int SerialChannelImpl::readImpl(char*& pBuffer)
{
	if (!_pConfig->getUseEOFImpl())
		throw InvalidAccessException();

	int bufSize = _pConfig->getBufferSizeImpl();
	int it = 1;

	if ((0 == bufSize) || (0 != pBuffer))
		throw InvalidArgumentException();

	std::string buffer;
	int readLen = 0;
	int readCount = 0;

	pBuffer = static_cast<char*>(std::calloc(bufSize, sizeof(char)));//! freed in parent call

	do
	{
		if (_leftOver.size())
		{
			readLen = _leftOver.size() > bufSize - readCount ? bufSize - readCount : _leftOver.size();
			std::memcpy(pBuffer + readCount, _leftOver.data(), readLen);
			if (readLen == _leftOver.size())
				_leftOver.clear();
			else
				_leftOver.assign(_leftOver, readLen, _leftOver.size() - readLen);
		}
		else
		{
			if ((readLen = read(_handle, pBuffer + readCount, bufSize - readCount)) < 0) 
				handleError(_pConfig->name());
			else if (0 == readLen) break;
		}

		poco_assert (readLen <= bufSize - readCount);
		
		buffer.assign(static_cast<char*>(pBuffer + readCount), readLen);
		size_t pos = buffer.find(_pConfig->getEOFCharImpl());
		if (pos != buffer.npos)
		{
			readCount += static_cast<int>(pos);
			_leftOver.assign(buffer, pos + 1, buffer.size() - pos - 1);
			break;
		}

		readCount += readLen;
		if (readCount >= bufSize)
		{
			bufSize *= ++it;
			pBuffer = static_cast<char*>(std::realloc(pBuffer, bufSize * sizeof(char)));
		}
	}while(true);

	return readCount;
}


int SerialChannelImpl::writeImpl(const char* buffer, int length)
{
    std::cout << "attempting to write: " << buffer << "< of length " << length << " to handle = " << _handle << std::endl;

	int written = write(_handle, buffer, length);

        std::cout << "WRITTEN ==" << written << std::endl;

    if (written < 0) {
        std::cout << "WRITTEN ==" << written << " this is an error: " << std::endl;
		handleError(_pConfig->name());

     } else if(written != length && 0 != written)
     {
        std::cout << "WRITTEN ==" << "written != length" <<  length << " this is an error: " << std::endl;
		handleError(_pConfig->name());
     }
        else if (0 == written)
		throw IOException("Error writing to " + _pConfig->name());



//	if ((written = write(_handle, buffer, length)) < 0 || 
//		((written != length) && (0 != written)))
//		handleError(_pConfig->name());
//	else if (0 == written)
//		throw IOException("Error writing to " + _pConfig->name());

	return written;
}


std::string& SerialChannelImpl::getErrorText(std::string& buf)
{
	switch (errno)
	{
	case EIO:
		buf = "I/O error.";
		break;
	case EPERM:
		buf = "Insufficient permissions.";
		break;
	case EACCES:
		buf = "File access denied.";
		break;
	case ENOENT:
		buf = "File not found.";
		break;
	case ENOTDIR:
		buf = "Not a directory.";
		break;
	case EISDIR:
		buf = "Not a file.";
		break;
	case EROFS:
		buf += "File is read only.";
		break;
	case ENOTEMPTY:
		buf = "Directory not empty.";
		break;
	case ENAMETOOLONG:
		buf = "Name too long.";
		break;
	case ENFILE:
	case EMFILE:
		buf = "Too many open files.";
		break;
	default:
		buf = "Unknown error.";
	}

	return buf;
}


void SerialChannelImpl::handleError(const std::string& path)
{
	switch (errno)
	{
	case EIO:
		throw IOException(path);
	case EPERM:
		throw FileAccessDeniedException("insufficient permissions", _pConfig->name());
	case EACCES:
		throw FileAccessDeniedException(_pConfig->name());
	case ENOENT:
		throw FileNotFoundException(_pConfig->name());
	case ENOTDIR:
		throw OpenFileException("not a directory", _pConfig->name());
	case EISDIR:
		throw OpenFileException("not a file", _pConfig->name());
	case EROFS:
		throw FileReadOnlyException(_pConfig->name());
	case ENAMETOOLONG:
		throw PathSyntaxException(_pConfig->name());
	case ENFILE:
	case EMFILE:
		throw FileException("too many open files", _pConfig->name());
	default:
		throw FileException(std::strerror(errno), _pConfig->name());
	}
}


} } } // namespace Poco::DeviceIO::Serial
