/////////////////////////////////////////////////////////////
// Class Description of BoardReaderMIMOSIS
//
// Dedicated to decode output files from the PXI acquisition system
//  dedicated to MIMOSIS sensors
//
//
/////////////////////////////////////////////////////////////
//
// created JB, 2021/04/21
// Last Modified:

#include "BoardReaderMIMOSIS.h"

ClassImp(BoardReaderMIMOSIS);

//------------------------------------------+-----------------------------------
BoardReaderMIMOSIS::BoardReaderMIMOSIS(int boardNumber, int nSensors, int triggerMode, int eventBuildingMode) {
  // Board creator
  // ==> OF COURSE the list of arguments might be changed if needed! <==

  fDebugLevel = 0; // set later by SetDebugLevel
  fBoardNumber = boardNumber;
  fNSensors = nSensors;
  fTriggerMode = triggerMode;
  fEventBuildingMode = eventBuildingMode;
  fVetoOverflow = false; // set later by SetVetoPixel

  // Needed ?
  // fSizeOfHeader;
  // fSizeOfTrailer;


  cout << "*****************************************" << endl;
  cout << "    < BoardReaderMIMOSIS constructor >      " << endl;
  cout << "*****************************************" << endl;
  cout << "Creating a BoardReaderMIMOSIS" << endl;
  cout << " * board nb: " << fBoardNumber << endl;
  cout << " * nb of sensors: " << fNSensors << endl;
  cout << " * trigger mode: " << fTriggerMode << endl;
  cout << " * event building mode: " << fEventBuildingMode << endl;
  cout << " * usage of veto for event with overflow: " << (fVetoOverflow?"YES":"NO") << endl;

  // Initialization
  fCurrentFileNumber = 0; // See also AddFileList
  fNoMoreFile = false;
  fCurrentEventNumber = 0;
  fCurrentTriggerNumber = 0;
  fTriggerCount = 0;
  fFrameCount = 0;
  fNEventsWithOverflow = 0;

  cout << "    < BoardReaderMIMOSIS constructor DONE >      " << endl;
}

//------------------------------------------+-----------------------------------
BoardReaderMIMOSIS::~BoardReaderMIMOSIS()
{

//  delete fCurrentEvent;
//  delete fInputFileName;

}

// --------------------------------------------------------------------------------------
bool BoardReaderMIMOSIS::AddFileList(string prefixFileName, int startIndex, int endIndex, string suffixFileName) {
  // Try opening each input files built from name=prefix+index+suffix
  // Keep only the good one
  // Work also for a single file with startIndex=endIndex
  //
  // Return true if at least one file can be opened

  bool rc = true;
  std::string fileName;

  if(fDebugLevel) cout <<  "BoardReaderMIMOSIS::AddFileList => Adding files with indexes from " << startIndex << " to " << endIndex << endl;

  // Try each filename looping on indexes, and keep in the list only the good ones
  for (size_t iFile = startIndex; iFile <= endIndex; iFile++) {
    fileName = prefixFileName + std::to_string(iFile) + suffixFileName;
    if(fDebugLevel>1) cout << "  trying file " << fileName << endl;
  	fRawFileStream.open( fileName);
  	if( fRawFileStream.fail() ) {
  		cout << endl << "ERROR BoardReaderMIMOSIS " << fBoardNumber << " file " << fileName << " does not exist!!" << endl;
      fRawFileStream.close();
  		rc = false;
  	}
  	else{
      fListInputFileNames.push_back( fileName);
  		cout << "  --> BoardReaderMIMOSIS " << fBoardNumber << " New file " << fileName << ", total of " << fListInputFileNames.size() << " files." << endl;
      fRawFileStream.close();
    }
  }
  cout << "#files OK = " << fListInputFileNames.size() << " out of " << endIndex-startIndex+1 << endl;

  // If there is only one file, try a filename without an index
  if ( !rc &&  endIndex==startIndex ) {
    fileName = prefixFileName + suffixFileName;
    if(fDebugLevel>1) cout << "  trying file " << fileName << endl;
  	fRawFileStream.open( fileName);
  	if( fRawFileStream.fail() ) {
  		cout << endl << "ERROR BoardReaderMIMOSIS " << fBoardNumber << " file " << fileName << " does not exist!!" << endl;
      fRawFileStream.close();
  		rc = false;
  	}
  	else{
      fListInputFileNames.push_back( fileName);
  		cout << "  --> BoardReaderMIMOSIS " << fBoardNumber << " New file " << fileName << ", total of " << fListInputFileNames.size() << " files." << endl;
      fRawFileStream.close();
    }
  }

  // if at least one file is OK, reopens the very first file
  if ( fListInputFileNames.size() == 0 ) {
    fNoMoreFile = true;
    return false;
  }

  fCurrentFileNumber = 0;
  return LookUpRawFile();

}

// --------------------------------------------------------------------------------------
bool BoardReaderMIMOSIS::LookUpRawFile() {
  // Try to open the next rawdata file

  // If the stream is open, close it
  if( fRawFileStream.is_open() ) {
    CloseRawFile();
    fCurrentFileNumber++;
  }

  // Try to go to the next file, if any are left
  if( fCurrentFileNumber < fListInputFileNames.size() ) {
    if(fDebugLevel) cout << "  --> BoardReaderMIMOSIS " << fBoardNumber << " New file to read " << fCurrentFileNumber << " < " << fListInputFileNames.size() << "." << endl;
    return OpenRawFile();
  }
  else { // Otherwise no more file, end the reading
    cout << "  -+-+- INFO BoardReaderMIMOSIS " << fBoardNumber << ": No more files to read " << fCurrentFileNumber << " >= " << fListInputFileNames.size() << " closing!" << endl;
    fNoMoreFile = true;
    return false;
  }

}

// --------------------------------------------------------------------------------------
bool BoardReaderMIMOSIS::OpenRawFile() {
  // Open the next rawdata file

  fRawFileStream.open( fListInputFileNames[fCurrentFileNumber]);
  bool b = fRawFileStream.fail();
  if (b == 0)  {
    cout << " -+-+- INFO BoardReaderMIMOSIS " << fBoardNumber << ": File " << fListInputFileNames[fCurrentFileNumber] << " opened." << endl;
  }
  else {
    cout << " -/-/- INFO BoardReaderMIMOSIS " << fBoardNumber << ": File " << fListInputFileNames[fCurrentFileNumber] << " not opened, rc = " << b << "." << endl;
  }
  return !b;

}

// --------------------------------------------------------------------------------------
bool BoardReaderMIMOSIS::CloseRawFile() {
  // Closes a File of a Run

  fRawFileStream.close();
  bool b = fRawFileStream.fail();
  if (b == 0)
    cout << " -+-+- INFO BoardReaderMIMOSIS " << fBoardNumber << ": File " << fCurrentFileNumber << " closed " << endl;
  else
    cout << " -/-/- INFO BoardReaderMIMOSIS " << fBoardNumber << ":  File " << fCurrentFileNumber << " not closed, rc = " << b << "(eof="<< fRawFileStream.eof() << ", bad="<< fRawFileStream.bad() <<"." << endl;
  return b;

}

// --------------------------------------------------------------------------------------

bool BoardReaderMIMOSIS::HasData( ) {
  // Try to find the pixel data for the next event in the raw data file
  //
  // ==> THIS MIGHT BE TOO SIMPLISTIC depending on the data format <==
  //
  //
  // If something goes wrong with the decoding or no more events,
  //  a flag (eventOK) is used to return "false"
  // Otherwise returns "true"
  //

  // -+-+- Initialization

  bool eventOK   = true;   // we start with a good event by default
  fCurrentEvent   = NULL;  // Allow to know wether data are correct
  fListOfTriggers.clear();  // and clear the vectors
  fListOfTimestamps.clear();
  fListOfFrames.clear();
  fListOfPixels.clear();

 if(fDebugLevel) {
   cout << endl ;
   cout << "BoardReaderMIMOSIS::HasData() fBoardNumber " << fBoardNumber << " BoardReaderMIMOSIS::HasData() - currentEvent " << fCurrentEventNumber << " currentTrigger " << fCurrentTriggerNumber << endl;
 }

 if( DecodeNextEvent() ) {

   if(fDebugLevel>1) PrintEventHeader();

   fCurrentEvent = new BoardReaderEvent( fCurrentEventNumber, fBoardNumber, 0, &fListOfPixels); // run number set to 0 for now
   fCurrentEvent->SetListOfTriggers( &fListOfTriggers);
   fCurrentEvent->SetListOfFrames( &fListOfFrames);
   if(fDebugLevel) cout << " fBoardNumber " << fBoardNumber << " created new event " << fCurrentEventNumber << " with " << fListOfPixels.size() << " pixels from " << fListOfTriggers.size() << " triggers and " << fListOfFrames.size() << " frames." << endl;
   fCurrentEventNumber++;

 } // getting next buffer was not OK
 else{
   eventOK = false;
   cout<<" -/-/- ERROR BoardReaderMIMOSIS::HasData() - Can't get next event !"<<endl;
 }

 // End
 // check there are still data to read
 return eventOK;
}

//------------------------------------------+-----------------------------------
bool BoardReaderMIMOSIS::DecodeNextEvent() {
  // THIS IS THE MAIN PLACE FOR ROMA's WORK!
  //
  // This method should decide whether
  //  there is still something to read in the current file
  //  or if the next file should be open (LookUpRawFile())
  //  or if there is nothing to read anymore ==> ready = false

  // Decoding status
  bool ready = false;

  if(fDebugLevel>2) printf( "  BoardReaderMIMOSIS board %d::DecodeNextEvent() trying with event %d\n", fBoardNumber, fCurrentEventNumber);

  // Emulate stupid event with few pixels/sensor, one frame, one trigger
  fListOfTriggers.push_back( fCurrentEventNumber);
  fListOfTimestamps.push_back( 0);
  fListOfFrames.push_back( fCurrentEventNumber);
  for (size_t iSensor = 0; iSensor < fNSensors; iSensor++) {
    // shoot 100 pixels in 1 row, which position depends on sensor
    for (size_t iCol = 0; iCol < 100; iCol++) {
      AddPixel( iSensor, 1, iSensor*10+50, iSensor*10+iCol);
    }
  }
  ready = true;


  return ready;

}

// --------------------------------------------------------------------------------------
void BoardReaderMIMOSIS::AddPixel( int iSensor, int value, int aLine, int aColumn, int aTime) {
   // Add a pixel to the vector of pixels
   // require the following info
   // - input = number of the sensors
   // - value = output value of this pixel
   // - line & column = position of the pixel in the matrix
   // - time = something related to the frame or the trigger

//  if (vi_Verbose<2) printf("BoardReaderMIMOSIS::Addpixel adding pixel for sensor %d with value %d line %d row %d\n", iSensor, value, aLine, aColumn, aTime);

  fListOfPixels.push_back( BoardReaderPixel( iSensor, value, aLine, aColumn, aTime) );

}

// --------------------------------------------------------------------------------------

void BoardReaderMIMOSIS::SetVetoPixel( int noiseRun) {
  // Select the required function to veto the pixel depending on the run number.
  //

  if( fDebugLevel) printf( "  BoardReaderMIMOSIS board %d::SetVetoPixel with noise run number %d\n", fBoardNumber, noiseRun);
  if (noiseRun>0 ) fTool.SetVetoPixel( noiseRun);

}

// --------------------------------------------------------------------------------------
void BoardReaderMIMOSIS::PrintEventHeader() {
  // Print Event Header

}

// --------------------------------------------------------------------------------------
void BoardReaderMIMOSIS::SkipNextEvent() {
  // This method might be useful to ignore the next event
  //

  if(fDebugLevel) printf("  BoardReaderMIMOSIS: %d skipping current event %d\n", fBoardNumber, fCurrentEventNumber);

}

// --------------------------------------------------------------------------------------
void BoardReaderMIMOSIS::PrintStatistics(ostream &stream) {
  // Print statistics on all the events read by this board

 stream << "***********************************************" << endl;
 stream << " Board MIMOSIS " << fBoardNumber << " found:" << endl;
 stream << fTriggerCount << " triggers read overall," << endl;
 stream << fCurrentEventNumber << " events in total," << endl;
 stream << fNEventsWithOverflow << " events with an overflow (veto overflow " << fVetoOverflow << ")," << endl;
 stream << fFrameCount << " frames read overall." << endl;
 stream << "***********************************************" << endl;

}
