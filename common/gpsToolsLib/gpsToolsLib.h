/*! \file gpsToolsLib.h
    \brief Some useful functions for playing with the GPS daya
    For now it's just a couple of functions, but it may well grow.
   
   July 2016 r.nichol@ucl.ac.uk
*/
#ifndef GPSTOOLSLIB_H
#define GPSTOOLSLIB_H


//! Fill a RawAdu5MBNStruct_t if inputStream contains one
/*!
  Fills the RawAdu5MBNStruct_t assuming that inputStream passes the test
  \return { 0 on success, negative on failure }
*/
int fillRawMBNStruct(char *inputStream, int length, RawAdu5MBNStruct_t *mbnPtr);


//! Fill a RawAdu5PBNStruct_t if inputStream contains one
/*!
  Fills the RawAdu5PBNStruct_t assuming that inputStream passes the test
  \return { 0 on success, negative on failure }
*/
int fillRawPBNStruct(char *inputStream, int length, RawAdu5PBNStruct_t *mbnPtr);


//! Fill a RawAdu5SNVStruct_t if inputStream contains one
/*!
  Fills the RawAdu5SNVStruct_t assuming that inputStream passes the test
  \return { 0 on success, negative on failure }
*/
int fillRawSNVStruct(char *inputStream, int length, RawAdu5SNVStruct_t *mbnPtr);

//! Fill a RawAdu5ATTStruct_t if inputStream contains one
/*!
  Fills the RawAdu5ATTStruct_t assuming that inputStream passes the test
  \return { 0 on success, negative on failure }
*/
int fillRawATTStruct(char *inputStream, int length, RawAdu5ATTStruct_t *mbnPtr); 


//! Calculate bytewise XOR
/*!
  Calculates a bytewise XOR to form a checksum
  \return { The checksum }
*/
unsigned char calculateBytewiseXOR(char *inputStream, int numBytes);


//! Calculate the sum of the words to form a checksum
/*!
  Calculates the sum of the words to form a checksum
  \return { The checksum }
*/
unsigned short calculateSumOfWords(char *inputStream, int numBytes);



#endif /* GPSTOOLSLIB_H */
