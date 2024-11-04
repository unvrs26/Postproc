#include <iostream>
#include <sstream>
#include <cstdlib>
#include <string>
#include <cmath>
#include <limits>
#include <vector>
#include <fstream>
#include <chrono> 
#include <dirent.h> 

using namespace std;
using namespace std::chrono;


////////////////////////////////////////////////////     GLOBALS     ////////////////////////////////////////////////////


// FILE RELATED

struct dirent *directoryEntry;
string inDir = "input/";
string outDir = "output/";
DIR *directory = opendir("input/");
vector<string> fileList = {};

string inputFile;
string outputFile;

string logFile = "logs/postproc.log";
// string arrayFile = "output/array.nc";


ofstream writeLog(logFile);
// ofstream writeArray(arrayFile);


// SETTINGS

const int loggingEnabled = 1;
const int debugLevel = 0;		// DEBUG LEVELS:
										// 1 - general indexing and comments
										// 2 - links
										// 3 - toolchange
										// 4 - modaxis


// FLAGS

int init = 1;
int gotLink = 0;
int writeLink = 0;
int gotLinkZ = 0;
int nextLineIsComment = 0;
int currentLineIsComment = 0;
int previousLineIsComment = 0;
int gotPreviousAxisValues = 0;
int toolChangeFlag = 0;
int modAxisFlag = 0;


// INDICES

int previousLineIndex = -2;
int currentLineIndex = -1;
int nextLineIndex = 0;


// AXIS LIMIT RELATED

const double detectRotDiff = 300.;
const double resetAxis = 77777.;
const string rotatingAxis = "B";

const double maxDepth = 15.;
const double liftZ = 30.;
const double liftZmin = 0.;

const double limitXmin = -20.;
const double limitYmin = -50.;
const double limitZmin = -80.;
const double limitAmin = 0.;

const double limitXmax = 190.;
const double limitYmax = 50.;
const double limitZmax = 250.;
const double limitAmax = 99.;

double modAxisValue;



// LINES AND LINE VALUES

string delimiter = " ";
size_t pos = 0;

string nextLine = "";
string nextLineInstruction = "";

string currentLine = "";
string currentLineInstruction = "";

string nextOperationHeader;

double nextX = resetAxis;
double nextY = resetAxis;
double nextZ = resetAxis;
double nextA = resetAxis;
double nextB = resetAxis;
string nextF = "";
string nextG = "";
string nextMa = "";
string nextMb = "";
string nextMc = "";
string nextS = "";
string nextT = "";

double currentX = resetAxis;
double currentY = resetAxis;
double currentZ = resetAxis;
double currentA = resetAxis;
double currentB = resetAxis;
string currentF = "";
string currentG = "";
string currentMa = "";
string currentMb = "";
string currentMc = "";
string currentS = "";
string currentT = "";

double previousX = resetAxis;
double previousY = resetAxis;
double previousZ = resetAxis;
double previousA = resetAxis;
double previousB = resetAxis;
string previousF = "";
string previousG = "";
string previousMa = "";
string previousMb = "";
string previousMc = "";
string previousS = "";
string previousT = "";

double mpl;
double absPMC;					
double linkZ;
double linkZtravel;	// = abs(previousZ - nextZ)
string linkLine;


// ARRAYS

vector<string> nextValues = {};
vector<string> currentValues = {};

vector<string> endSequenceArray =
{
	"(FINISH)",
	"M5 M9",
	"G00 Z200",
	"G00 X150",
	"G00 Y0",
	"G00 B0",
	"G00 A0",
	"M30"
};

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////


int main()
{
	
	if(directory == NULL)
	{ 
		cout << "Could not open current directory" << endl;
		cin.get();
		return 0;
	}
	
	while((directoryEntry = readdir(directory)) != NULL)
	{
		fileList.push_back(directoryEntry -> d_name);
	}
	closedir(directory);



	cout << "List of files in queue:" << endl;
	if(loggingEnabled == 1){writeLog << "List of files in queue:" << endl;}
	
	for(int h=2; h<fileList.size(); h++)
	{
		cout << fileList[h] << endl;
		if(loggingEnabled == 1){writeLog << fileList[h] << endl;}
	}
	cout << endl << endl << "____________________________________"<< endl << endl;
	if(loggingEnabled == 1){writeLog << endl << endl << "____________________________________" << endl << endl;}

	
////////////////////////////////////////////////////////////////////////////////
	
	
	for(int k=2; k<fileList.size(); k++)
	{
		auto start = chrono::high_resolution_clock::now();

		string inputFile = inDir+fileList[k];
		string outputFile = outDir+fileList[k];
		ofstream writeFile(outputFile);
		
		if(loggingEnabled == 1)
		{
			writeLog << "Intput file: " << inputFile << endl;
			writeLog << "Output file: " << outputFile << endl;
			writeLog << "Messages:" << endl;
		}
		cout << "Input file:  " << inputFile << endl;
		cout << "Output file: " << outputFile << endl;
		cout << "Messages:" << endl;
		
//		cin.get();																						// debug
			
		string line;
		ifstream readFile;
		readFile.open(inputFile);
			
	   if(!readFile.is_open())																			// open error
		{
			perror("File could not be opened");
			writeLog << "File could not be opened" << endl;
			cin.get();
			exit(EXIT_FAILURE);
	   }
	   
	   vector<string> gCode = {};
		
		int lineIdx = 0;
	
		while(getline(readFile, line))
		{
			size_t T1 = line.find("T1");
			size_t M03 = line.find("M03");
			size_t M30 = line.find("M30");
			size_t M5 = line.find("M5");
			size_t M9 = line.find("M9");
	
			if(T1 != string::npos){if(loggingEnabled == 1)writeLog << "input line " << lineIdx << ":\tT1 value was found and removed from input stream" << endl;}
			else if(M03 != string::npos){if(loggingEnabled == 1)writeLog << "input line " << lineIdx << ":\tM03 value was found and removed from input stream" << endl;}
			else if(M30 != string::npos){if(loggingEnabled == 1)writeLog << "input line " << lineIdx << ":\tM30 value was found and removed from input stream" << endl;}
			else if(M5 != string::npos){if(loggingEnabled == 1)writeLog << "input line " << lineIdx << ":\tM5 value was found and removed from input stream" << endl;}
			else if(M9 != string::npos){if(loggingEnabled == 1)writeLog << "input line " << lineIdx << ":\tM9 value was found and removed from input stream" << endl;}
	
			else
			{
				if(line.substr(0, 1) == " ")																// deleting initial spaces
				{
					line.erase(0, 1);
				}
				gCode.push_back(line);
			}
			
			lineIdx++;
		}

	
//----------------------------------------------------------------------------//
//---------------------------READING FILES BEGIN------------------------------//
//----------------------------------------------------------------------------//
	
			
	
								//----------------------------//
	//-----------------------PRIMARY PROCESSING LOOP------------------------------//
								//----------------------------//
	
	
		for(int i=0; i<gCode.size(); i++)
		{
			if(init == 1){goto reading;}																// if there's no data yet, go to first read cycle
	
	
	
	//----------------------------------------------------------------------------//
	//---------------------------WRITING VALUES BEGIN-----------------------------//
	//----------------------------------------------------------------------------//
	
	
	
								//----------------------------//
	//----------------------WRITING AXIS CONTROL BEGIN----------------------------//
								//----------------------------//
	
	
			if(currentLineIsComment == 0)
			{
	
	//----------------------------INIT SEQUENCE-----------------------------------//
	
				if(currentLineIndex == 2)																// AFTER INIT COMMENTS (CURRENTLINEINDEX > 2) INSERTING INIT
				{
					writeFile << "(INIT)" << endl;
					writeFile << "M6 T1" << endl;
					writeFile << "G00 Z200" << endl;
					writeFile << "G00 A" << nextA << endl;
					writeFile << "G00 B" << nextB << endl;
					writeFile << "G00 X" << nextX << " Y" << nextY << endl;
					writeFile << "M00" << endl;														// one deep breath before the plunge
					writeFile << "G00 Z" << nextZ << endl;
	
					writeFile << gCode[1] << endl;
					
					gotLink = 1;
				}
	
	//------------------------LINK CALCULATION BEGIN------------------------------//
	
	
				if(writeLink == 1 && toolChangeFlag == 0)											// regular link
				{
					linkZtravel = abs(previousZ - nextZ);
	
					if(debugLevel == 2){writeFile << endl << "DEBUG: previousZ: " << previousZ << " - nextZ: " << nextZ << endl << endl;}
					if(nextZ == resetAxis){nextZ = previousZ;}
					
					//     Z WILL RISE     //
					if(previousZ < nextZ)
					{
						gotLinkZ = 1;
						linkZ = nextZ + liftZ;
											
						if(linkZ > limitZmax)
						{
							writeLog << "line " << currentLineIndex << ":\tSEVERE: calculated Z value exceeds machine maximum" << endl;
							cout << "line " << currentLineIndex << ":\tSEVERE! Calculated Z value exceeds machine maximum" << endl;
							cin.get();
							exit(EXIT_FAILURE);
						}
						writeFile << "(LINK - Z RISES)" << endl;					
					}
							
					//     Z WILL DROP     //
					else if(previousZ > nextZ)
					{
						gotLinkZ = 1;
						linkZ = previousZ + liftZ;
						
						if(linkZ < limitZmin)
						{
							writeLog << "line " << currentLineIndex << ":\tSEVERE: calculated Z value is less than machine minimum" << endl;
							cout << "line " << currentLineIndex << ":\tSEVERE! Calculated Z value is less than machine minimum" << endl;
							cin.get();
							exit(EXIT_FAILURE);
						}
						writeFile << "(LINK - Z DROPS)" << endl;
					}
					
					//     Z STAYS THE SAME     //
					else if(previousZ == nextZ)
					{
						gotLinkZ = 1;
						linkZ += liftZ;
	
						if(linkZ > limitZmax)
						{
							writeLog << "line " << currentLineIndex << ":\tSEVERE: calculated Z value exceeds machine maximum" << endl;
							cout << "line " << currentLineIndex << ":\tSEVERE! Calculated Z value exceeds machine maximum" << endl;
							cin.get();
							exit(EXIT_FAILURE);
						}
						writeFile << "(LINK - Z STAYS)" << endl;
					}
					
					if(gotLinkZ == 1)
					{
						writeFile << "G00 Z" << linkZ << endl;
						gotLinkZ = 0;
					}
	
					writeFile << "G00 A" << nextA << endl;
					writeFile << "G00 B" << nextB << endl;
					writeFile << "G00 X" << nextX << " Y" << nextY << endl;
					writeFile << "G00 Z" << nextZ << endl;
	
					writeFile << nextOperationHeader << endl;
					
					gotLink = 1;
					writeLink = 0;
				}
				
	//---------------------------WRITING TOOLCHANGE-------------------------------//
				
				if(writeLink == 1 && toolChangeFlag == 1)											// toolchange
				{
					//     TOOLCHANGE SEQUENCE     //
					writeFile << "M5 M9" << endl;
					writeFile << "G00 Z200" << endl;
					writeFile << "G00 X150 Y-20" << endl;
					writeFile << "G00 A90" << endl;
					writeFile << "M00" << endl;
					writeFile << "(RE-INIT)" << endl;
					writeFile << "M6 T2" << endl;
					writeFile << "M03 S7000" << endl;
					writeFile << "G00 X" << nextX << " Y" << nextY << endl;
					writeFile << "G00 A" << nextA << endl;
					writeFile << "G00 B" << nextB << endl;
					writeFile << "M00" << endl;
					writeFile << "G00 Z" << nextZ << endl;
					
					writeFile << gCode[nextLineIndex - 3] << endl;			//continue here
					writeFile << gCode[nextLineIndex - 2] << endl;			//continue here
					
					if(debugLevel == 3)
					{
						writeFile << "\nDEBUG: current line at the end of building TOOLCHANGE   -   " << currentLine << endl;
						writeFile << "DEBUG: currentLineIndex   -   " << currentLineIndex << endl;
						writeFile << "DEBUG: next line at the end of building TOOLCHANGE     -    " << nextLine << endl;
						writeFile << "DEBUG: nextLineIndex      -    " << nextLineIndex << endl;
					}
					
					gotLink = 1;
					writeLink = 0;
					toolChangeFlag = 0;
				}
	
								//----------------------------//
	//--------------CURRENT AXIS (EXCEPT FOR AXIS B) VALUES PROCESSING------------//
								//----------------------------//
	
				currentX = resetAxis;
				currentY = resetAxis;
				currentZ = resetAxis;
				currentA = resetAxis;
				currentB = resetAxis;
				currentF = "";
				currentG = "";
				currentMa = "";
				currentMb = "";
				currentMc = "";
				currentS = "";
				currentT = "";
	
				currentX = nextX;
				currentY = nextY;
				currentZ = nextZ;
				currentA = nextA;
				// currentB = nextB;
				currentF = nextF;
				currentG = nextG;
				currentMa = nextMa;
				currentMb = nextMb;
				currentMc = nextMc;
				currentS = nextS;
				currentT = nextT;
				
	
	//--------------------CURRENT AXIS B VALUE PROCESSING-------------------------//
	
				
				mpl = abs(currentB) / 360.;
				absPMC = abs(currentB - nextB);					
							
				if(absPMC > detectRotDiff && currentB != resetAxis)
				{
					if(mpl > 1 || mpl == 1)
					{
						if(currentB < nextB){currentB = nextB - (mpl * 360.);}
						else if(currentB > nextB){currentB = nextB + (mpl * 360.);}
						else
						{
							if(loggingEnabled == 1){writeLog << endl << "line " << currentLineIndex << ":\tError! Axis B: No such case declared. Terminating." << endl;}
							perror("No such case declared");
							break;
						}
					}
					else
					{
						if(currentB < nextB){currentB = nextB - 360.;}
						else if(currentB > nextB){currentB = nextB + 360.;}
						else
						{
							if(loggingEnabled == 1){writeLog << endl << "line " << currentLineIndex << ":\tError! Axis B: No such case declared. Terminating." << endl << endl;}
							perror("No such case declared");
							break;
						}
					}
				}
				else{currentB = nextB;}
					
				nextValues.clear();
	
				if(currentMa != previousMa && currentMa != "")							// deleting M5, M9 and M30 values
				{
					if(currentMa == "M5" || currentMa == "M9" || currentMa == "M30")
					{
						cout << "currentMa: " << currentMa << endl;
					}
				}
	
		
	//-------------------------BUILDING OUTPUT VECTOR-----------------------------//
	
		
				vector <string> outputLine = {};			// if you make this global, it WILL mess your day up
				
				if(currentMa != ""){outputLine.push_back(currentMa);}
				if(currentMb != ""){outputLine.push_back(currentMb);}
				if(currentMc != ""){outputLine.push_back(currentMc);}
				if(currentT != ""){outputLine.push_back(currentT);}
				if(currentS != ""){outputLine.push_back(currentS);}
				
				if(currentF != "")
				{
					if(currentG != ""){outputLine.push_back(currentG);}
					else{outputLine.push_back("G01");}
				}
				
				else if(previousLineIsComment == 1 && currentG != "")
				{
					outputLine.push_back("G01");
				}
				
				if(currentX != resetAxis)				// if you want to write the changes ONLY, change condition to   currentX != previousX && currentX != ""
				{
					std::ostringstream ss;
					ss << currentX;
					string writeX("X"+ss.str());
					outputLine.push_back(writeX);
				}
				
				if(currentY != resetAxis)
				{
					std::ostringstream ss;
					ss << currentY;
					string writeY("Y"+ss.str());
					outputLine.push_back(writeY);
				}
				
				if(currentZ != resetAxis)
				{
					std::ostringstream ss;
					ss << currentZ;
					string writeZ("Z"+ss.str());
					outputLine.push_back(writeZ);
				}
				
				if(currentA != resetAxis)
				{
					std::ostringstream ss;
					ss << currentA;
					string writeA("A"+ss.str());
					outputLine.push_back(writeA);
				}
				
				if(currentB != resetAxis)
				{
					std::ostringstream ss;
					ss << currentB;
					string writeB("B"+ss.str());
					outputLine.push_back(writeB);
				}
				
				if(currentF != ""){outputLine.push_back(currentF);}
	
		
	//------------------BARFING CURRENTLINE INTO A STRINGSTREAM-------------------//
	
				
				if(!outputLine.empty())
				{
					std::ostringstream result;
					writeFile << " ";
					for(string opt : outputLine)
					{
						result << opt << " ";
					}
					currentLine = (result.str());
					writeFile << currentLine << endl;
				}
	
				//     PROCESSING INDEXES     //
			
				nextLineIndex++;
				
				if(previousLineIsComment == 1)
				{
					previousLineIndex++;				
				}
				
				else
				{
					previousLineIndex = nextLineIndex - 2;
				}
	
				if(debugLevel == 1)
				{
					writeFile << "DEBUG: previousLineIndex = " << previousLineIndex << endl;			// DEBUGBURGER
					writeFile << "DEBUG: currentLineIndex = " << currentLineIndex << endl;
					writeFile << "DEBUG: nextLineIndex = " << nextLineIndex << endl << endl;
				}
		
	
	//-------------------SETTING PREVIOUS VALUES AND FLAGS------------------------//
		
				if(currentX != resetAxis){previousX = currentX;}
				if(currentY != resetAxis){previousY = currentY;}
				if(currentZ != resetAxis){previousZ = currentZ;}
				if(currentA != resetAxis){previousA = currentA;}
				if(currentB != resetAxis){previousB = currentB;}
				previousF = currentF;
				previousG = currentG;
				previousS = currentS;
				previousT = currentT;
				previousMa = currentMa;
				previousMb = currentMb;
				previousMc = currentMc;
				
				previousLineIsComment = 0;
				gotPreviousAxisValues = 1;
			}
	
	
								//----------------------------//
	//----------------------WRITING AXIS CONTROL END------------------------------//
								//----------------------------//
	
	
	
								//----------------------------//
	//-------------------WRITING LINKS AND COMMENTS BEGIN-------------------------//
								//----------------------------//
	
		
			if(currentLineIsComment == 1)
			{
	
	//-----------------GOT LINK AND NEXT LINE AIN'T NO COMMENT--------------------//
	
				
				if(gotLink == 1 && (gCode[nextLineIndex].substr(0, 1) != "("))
				{
					gotLink = 0;
					writeLink = 1;																			// cocking hammer for writeLink
					if(debugLevel == 2){writeFile << "DEBUG: link is prepared, writeLink is triggered, BUT next line is NOT comment!" << endl;}
					nextOperationHeader = currentLine;												// cocking hammer for header of operation AFTER link
				}
	
							//-----------------------------------//
	//-------------------------CATCHING COMMENT TYPES-----------------------------//
							//-----------------------------------//
	
				else if(gotLink == 1 && (gCode[nextLineIndex].substr(0, 1) == "("))
				{
					if(debugLevel == 2)
					{
						writeFile << "DEBUG: index processing at end of Writing Comment  -  previousLineIndex = " << previousLineIndex << endl;
						writeFile << "DEBUG: gotLink == 1, AND nextLineIsComment = 1  -           currentLine = " << currentLine << endl;
						writeFile << "DEBUG: index processing at end of Writing Comment  -   currentLineIndex = " << currentLineIndex << endl;
						writeFile << "DEBUG: gotLink == 1, AND nextLineIsComment = 1  -              nextLine = " << nextLine << endl;
						writeFile << "DEBUG: index processing at end of Writing Comment  -      nextLineIndex = " << nextLineIndex << endl;
						writeFile << "DEBUG: gotLink == 1, AND nextLineIsComment = 1  -   nextOperationHeader = " << nextOperationHeader << endl;
					}
					
	//--------------------------WRITING TOOLCHANGE--------------------------------//
					
					//if(gCode[nextLineIndex+1].find("T") && ((gCode[nextLineIndex-1].substr(0, 8)) != "(MODAXIS"))
					
					size_t T2 = gCode[nextLineIndex+1].find("T2");
					if(T2 != string::npos)
					{
						writeFile << "(TOOLCHANGE)" << endl;
						writeLink = 1;
						toolChangeFlag = 1;
						nextOperationHeader = currentLine;
						if(debugLevel == 3){writeFile << "DEBUG: current line then TOOLCHANGE is triggered " << currentLine << endl;}
					}
					if(debugLevel == 3){writeFile << "DEBUG: indexes are set " << currentLine << endl;}
	
	//-----------------------------WRITING MODAXIS--------------------------------//
	
					if((gCode[nextLineIndex-1].substr(0, 8)) == "(MODAXIS")					// MODAXIS
					{
						if(debugLevel == 4){writeFile << "DEBUG: Got MODAXIS" << endl;}
						writeFile << nextOperationHeader << endl;
					}
	
	//------------------------CAN'T REMEMBER WHAT THIS IS-------------------------//	PROBABLY A DEBUG MESSAGE IN FRONT OF INIT COMMENTS FFS!
	
					else
					{
						writeLink = 1;
						if(debugLevel == 2){writeFile << "DEBUG: writeLink is 1, nextOperationHeader = currentLine" << endl;}
						nextOperationHeader = currentLine;
					}
					
					gotLink = 0;
				}
	
	//----------------------------ELSE, LIKE INIT---------------------------------//
	
				else
				{
					
					if(gCode[nextLineIndex].substr(0,1) == "(")
					{
						if(debugLevel == 1){writeFile << "DEBUG: init" << endl;}
						writeFile << currentLine << endl;
					}
					else{if(debugLevel == 1){writeFile << "DEBUG: not init" << endl;}}
				}
	
				if(debugLevel == 1)
				{
					writeFile << "DEBUG: index processing at end of Writing Comment  -  previousLineIndex = " << previousLineIndex << endl;
					writeFile << "DEBUG: index processing at end of Writing Comment  -  currentLineIndex = " << currentLineIndex << endl;
					writeFile << "DEBUG: index processing at end of Writing Comment  -  nextLineIndex = " << nextLineIndex << endl;
				}
				previousLineIsComment = 1;
			}		
	
	
	
	//----------------------------------------------------------------------------//
	//---------------------------WRITING VALUES END-------------------------------//
	//----------------------------------------------------------------------------//
	
	
	
	//----------------------------------------------------------------------------//
	//----------------------READING AND PROCESSING BEGIN--------------------------//
	//----------------------------------------------------------------------------//
	
			reading:
			
			init = 0;
	
			if(toolChangeFlag == 1)																		//toolchange
			{
				nextLineIndex += 2;
				nextLine = gCode[nextLineIndex];
			}
			else{nextLine = gCode[nextLineIndex];}
			
		
	//--------------------------RESETTING VALUES----------------------------------//
			
			currentX = resetAxis;
			currentY = resetAxis;
			currentZ = resetAxis;
			currentA = resetAxis;
			currentB = resetAxis;
			currentF = "";
			currentG = "";
			currentMa = "";
			currentMb = "";
			currentMc = "";
			currentS = "";
			currentT = "";
			
								//----------------------------//
	//--------------------------READING AXIS LINE---------------------------------//
								//----------------------------//
			
			
			if(nextLine.substr(0, 1) != "(")
			{
				currentLineIsComment = 0;
	
				while((pos = nextLine.find(delimiter)) != string::npos)						// Separating values into strings
				{
					pos = nextLine.find(delimiter);
					nextLineInstruction = nextLine.substr(0, pos);
					nextValues.push_back(nextLineInstruction);
					nextLine.erase(0, pos + delimiter.length());
				}
				nextValues.push_back(nextLine);														// NEXTLINE ARRAY IS READY
	
	
	//--------------------------PARSING AXIS LINE---------------------------------//
	
				//-----RESETTING VALUES FROM PREVIOUS LINES-----//
							
				nextX = resetAxis;
				nextY = resetAxis;
				nextZ = resetAxis;
				nextA = resetAxis;
				nextB = resetAxis;
				nextF = "";
				nextG = "";
				nextMa = "";
				nextMb = "";
				nextMc = "";
				nextS = "";
				nextT = "";
	
	//-------------------------SORTING AXIS VALUES--------------------------------//
			
				for(int i=0; i<nextValues.size(); i++)
				{
					if(nextValues[i].substr(0, 1) == "X")
					{
						nextValues[i].erase(0, 1);
						nextX = stod(nextValues[i], &pos);
						
						if(loggingEnabled == 1)
						{
							if(limitXmin > nextX){writeLog << "line " << nextLineIndex << ":\tWARNING! Axis X value is less than machine minimum" << endl;}
							if(nextX > limitXmax){writeLog << "line " << nextLineIndex << ":\tWARNING! Axis X value exceeds machine maximum" << endl;}
							if(nextX < 0){writeLog << "line " << nextLineIndex << ":\tWARNING! X- value detected" << endl;}					
							if(nextX == 0){writeLog << "line " << nextLineIndex << ":\tWARNING! X0 value detected" << endl;}
						}
					}
					
					else if(nextValues[i].substr(0, 1) == "Y")
					{
						nextValues[i].erase(0, 1);
						nextY = stod(nextValues[i], &pos);
	
						if(loggingEnabled == 1)
						{
							if(limitYmin > nextY){writeLog << "line " << nextLineIndex << ":\tWARNING! Axis Y value is less than machine minimum" << endl;}					
							if(nextY > limitYmax){writeLog << "line " << nextLineIndex << ":\tWARNING! Axis Y value exceeds machine maximum" << endl;}					
						}
					}
					
					else if(nextValues[i].substr(0, 1) == "Z")
					{
						nextValues[i].erase(0, 1);
						nextZ = stod(nextValues[i], &pos);
	
						if(loggingEnabled == 1)
						{
							if(limitZmin > nextZ){writeLog << "line " << nextLineIndex << ":\tWARNING! Axis Z value is less than machine minimum" << endl;}					
							if(nextZ > limitZmax){writeLog << "line " << nextLineIndex << ":\tWARNING! Axis Z value exceeds machine maximum" << endl;}					
						}
					}
					
					else if(nextValues[i].substr(0, 1) == "A")
					{
						nextValues[i].erase(0, 1);
						nextA = stod(nextValues[i], &pos);
	
						if(nextA < 0)
						{
							nextA = abs(nextA);
							if(loggingEnabled == 1){writeLog << "line " << nextLineIndex << ":\tWARNING! Axis A- value corrected" << endl;}
						}
						
						if(loggingEnabled == 1)
						{
							if(limitAmin > nextA){writeLog << "line " << nextLineIndex << ":\tWARNING! Axis A value is less than machine minimum" << endl;}					
							if(nextA > limitAmax){writeLog << "line " << nextLineIndex << ":\tWARNING! Axis A value exceeds machine maximum" << endl;}					
							if(nextA == 0){writeLog << "line " << nextLineIndex << ":\tWARNING! A0 value detected" << endl;}					
						}
					}
					
					else if(nextValues[i].substr(0, 1) == "B")
					{
						nextValues[i].erase(0, 1);
						nextB = stod(nextValues[i], &pos);
						if(nextB < 0)
						{
							nextB = abs(nextB);
							if(loggingEnabled == 1){writeLog << "line " << nextLineIndex << ":\tWARNING! Axis B- value corrected" << endl;}
						}
						
						if(modAxisFlag == 1){nextB += modAxisValue;}
					}
					
					else if(nextValues[i].substr(0, 1) == "F"){nextF = nextValues[i];}
					else if(nextValues[i].substr(0, 3) == "G00" || nextValues[i].substr(0, 3) == "G01"){nextG = nextValues[i];}
	
					else if(nextValues[i].substr(0, 1) == "M")
					{
						if(nextMa == ""){nextMa = nextValues[i];}
						else if(nextMa != "" && nextMb == ""){nextMb = nextValues[i];}
						else if(nextMa != "" && nextMb != ""){nextMc = nextValues[i];}
						else if(nextMa != "" && nextMb != "" && nextMc != "")
						{
							if(loggingEnabled == 1){writeLog << "line " << nextLineIndex << ":\tCheck gCode, there might be an error" << endl;}
							cout << "Check gCode, there might be an error on line " << nextLineIndex << "." << endl;
							break;
						}
					}
										
					else if(nextValues[i].substr(0, 1) == "S"){nextS = nextValues[i];}
					else if(nextValues[i].substr(0, 1) == "T"){nextT = nextValues[i];}
					else if(nextValues[i].empty() && !(nextValues.back()).empty())
					{
						if(loggingEnabled == 1){writeLog << "line " << nextLineIndex << ":\tFound empty line" << endl;}
						cout << "Found empty line" << endl;
					}
					
					else
					{
						if(loggingEnabled == 1){writeLog << "line " << nextLineIndex << ":\tFound something irregular" << endl;}
						cout << "Found something irregular" << endl;
					}			
				}
				currentLineIndex++;			
			}		
			
								//----------------------------//
	//------------------------READING AXIS LINE END-------------------------------//
								//----------------------------//
	
	
	
								//----------------------------//
	//-------------------------READING COMMENT LINE-------------------------------//
								//----------------------------//
			
			else
			{
				currentLine = nextLine;
				currentLineIsComment = 1;
	
				currentLineIndex++;
				nextLineIndex++;
	
	
	//--------------------------------MODAXIS-------------------------------------//
	
	
				if(currentLine.substr(0, 8) == "(MODAXIS")
				{
					nextOperationHeader = currentLine;
					if(loggingEnabled == 1){writeLog << "line " << currentLineIndex << ":\tMODAXIS found, ";}
					modAxisFlag = 1;				
					currentLine.erase(0, 9);
					currentLine.pop_back();
					modAxisValue = stod(currentLine);
					if(loggingEnabled == 1){writeLog << "modAxisValue = " << modAxisValue << endl;}				
				}
	
	//------------------------------NOT MODAXIS-----------------------------------//	HERE WOULD BE A GOOD PLACE TO HANDLE ALL THE VARIOUS CASES LIKE LINKS
	
				else if(currentLineIndex > 1) // && (gCode[nextLineIndex+1].substr(0, 1) != "("))
				{			   
					if(debugLevel == 2){writeFile << "DEBUG: Got link!" << endl;}
					gotLink = 1;				
				}
			}
	
	//----------------------------------------------------------------------------//
	//------------------------READING AND PROCESSING END--------------------------//
	//----------------------------------------------------------------------------//
		
		}
	
	//---------------------------------LOOP END-----------------------------------//
	
	
		for(string n : endSequenceArray){writeFile << n << endl;}							// writing endSeq
		cout << "EOF" << endl << endl;
		if(loggingEnabled == 1){writeLog << "EOF" << endl << endl;}
		
		auto stop = chrono::high_resolution_clock::now(); 
		auto duration = duration_cast<milliseconds>(stop - start);
		
		cout << "Total number of lines: " << nextLineIndex + endSequenceArray.size() << endl;
		cout << "Processing time: " << duration.count() << " ms"<< endl;
		cout << "____________________________________" << endl << endl;
	
		if(loggingEnabled == 1)
		{
			writeLog << "Total number of lines: " << nextLineIndex + endSequenceArray.size() << endl;
			writeLog << "Processing time: " << duration.count() << " ms"<< endl;
			writeLog << "____________________________________" << endl << endl;
		}
		

		// PREPARING RERUN

		gCode.clear();
		
		nextLine = "";
		nextLineInstruction = "";
		currentLine = "";
		currentLineInstruction = "";
		nextOperationHeader = "";
		
		nextX = resetAxis;
		nextY = resetAxis;
		nextZ = resetAxis;
		nextA = resetAxis;
		nextB = resetAxis;
		nextF = "";
		nextG = "";
		nextMa = "";
		nextMb = "";
		nextMc = "";
		nextS = "";
		nextT = "";
		
		currentX = resetAxis;
		currentY = resetAxis;
		currentZ = resetAxis;
		currentA = resetAxis;
		currentB = resetAxis;
		currentF = "";
		currentG = "";
		currentMa = "";
		currentMb = "";
		currentMc = "";
		currentS = "";
		currentT = "";
		
		previousX = resetAxis;
		previousY = resetAxis;
		previousZ = resetAxis;
		previousA = resetAxis;
		previousB = resetAxis;
		previousF = "";
		previousG = "";
		previousMa = "";
		previousMb = "";
		previousMc = "";
		previousS = "";
		previousT = "";
		
		linkLine = "";
		
		mpl = resetAxis;
		absPMC = resetAxis;
		linkZ = resetAxis;
		linkZtravel = resetAxis;


		// FLAGS
				
		init = 1;
		gotLink = 0;
		writeLink = 0;
		gotLinkZ = 0;
		nextLineIsComment = 0;
		currentLineIsComment = 0;
		previousLineIsComment = 0;
		gotPreviousAxisValues = 0;
		toolChangeFlag = 0;
		modAxisFlag = 0;
		
		
		// INDICES
		
		previousLineIndex = -2;
		currentLineIndex = -1;
		nextLineIndex = 0;
	
	}
	
	cout << endl << endl << "Finished processing " << fileList.size()-2 << "files." << endl;
	cout << "Press any key to quit" << endl;

	if(loggingEnabled)
	{
		writeLog << endl << endl << "Finished processing " << fileList.size()-2 << " files" << endl;
		writeLog << "Press any key to quit" << endl;
	}
	cin.get();
}

