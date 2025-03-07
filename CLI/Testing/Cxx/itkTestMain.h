/*=========================================================================
 *
 *  Copyright Insight Software Consortium
 *
 *  Licensed under the Apache License, Version 2.0 (the "License");
 *  you may not use this file except in compliance with the License.
 *  You may obtain a copy of the License at
 *
 *         http://www.apache.org/licenses/LICENSE-2.0.txt
 *
 *  Unless required by applicable law or agreed to in writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS,
 *  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions and
 *  limitations under the License.
 *
 *=========================================================================*/
/*=========================================================================
 *
 *  Portions of this file are subject to the VTK Toolkit Version 3 copyright.
 *
 *  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
 *
 *  For complete copyright, license and disclaimer of warranty information
 *  please refer to the NOTICE file at the top of the ITK source tree.
 *
 *=========================================================================*/
#ifndef __itkTestMain_h
#define __itkTestMain_h

// This file is used to create TestDriver executables
// These executables are able to register a function pointer to a string name
// in a lookup table.   By including this file, it creates a main function
// that calls RegisterTests() then looks up the function pointer for the test
// specified on the command line.
#include "itkWin32Header.h"
#include <map>
#include <string>
#include <iostream>
#include <fstream>
#include "itkImage.h"
#include "itkImageFileReader.h"
#include "itkImageFileWriter.h"
#include "itkImageRegionConstIterator.h"
#include "itkSubtractImageFilter.h"
#include "itkRescaleIntensityImageFilter.h"
#include "itkExtractImageFilter.h"
#include "itkFloatingPointExceptions.h"
#include "itkTestingComparisonImageFilter.h"
#include "itkVectorIndexSelectionCastImageFilter.h"
#include "itksys/SystemTools.hxx"
#include "itkIntTypes.h"
#ifdef HAS_ITK_FACTORY_REGISTRATION
# include <itkFactoryRegistration.h>
#endif

#define ITK_TEST_DIMENSION_MAX 6

typedef int ( *MainFuncPointer )(int, char *[]);
std::map<std::string, MainFuncPointer> StringToTestFunctionMap;

#define REGISTER_TEST(test)       \
  extern int test(int, char *[]); \
  StringToTestFunctionMap[#test] = test

int RegressionTestImage(const char *testImageFilename, const char *baselineImageFilename, int reportErrors,
                        double intensityTolerance, ::itk::SizeValueType numberOfPixelsTolerance = 0,
                        unsigned int radiusTolerance = 0);

std::map<std::string, int> RegressionTestBaselines(char *);

void RegisterTests();

void PrintAvailableTests()
{
  std::cout << "Available tests:\n";
  std::map<std::string, MainFuncPointer>::iterator j = StringToTestFunctionMap.begin();
  int                                              i = 0;

  while( j != StringToTestFunctionMap.end() )
    {
    std::cout << i << ". " << j->first << "\n";
    ++i;
    ++j;
    }
}

int main(int ac, char *av[])
{
  itk::FloatingPointExceptions::Enable();

  double       intensityTolerance  = 0.0001;
  unsigned int numberOfPixelsTolerance = 0;
  unsigned int radiusTolerance = 0;
  bool         expectFail = false;

  typedef std::pair<char *, char *> ComparePairType;
  std::vector<ComparePairType> compareList;

#ifdef HAS_ITK_FACTORY_REGISTRATION
  itk::itkFactoryRegistration();
#endif

  RegisterTests();
  std::string testToRun;
  if( ac < 2 )
    {
    PrintAvailableTests();
    std::cout << "To run a test, enter the test number: ";
    int testNum = 0;
    std::cin >> testNum;
    std::map<std::string, MainFuncPointer>::iterator j = StringToTestFunctionMap.begin();
    int                                              i = 0;
    while( j != StringToTestFunctionMap.end() && i < testNum )
      {
      ++i;
      ++j;
      }

    if( j == StringToTestFunctionMap.end() )
      {
      std::cerr << testNum << " is an invalid test number\n";
      return -1;
      }
    testToRun = j->first;
    }
  else
    {
    while( ac > 0 && testToRun.empty() )
      {
      if( strcmp(av[1], "--with-threads") == 0 )
        {
        int numThreads = atoi(av[2]);
        av += 2;
        ac -= 2;
        }
      else if( strcmp(av[1], "--without-threads") == 0 )
        {
        av += 1;
        ac -= 1;
        }
      else if( ac > 3 && strcmp(av[1], "--compare") == 0 )
        {
        compareList.push_back( ComparePairType(av[2], av[3]) );
        av += 3;
        ac -= 3;
        }
      else if( ac > 2 && strcmp(av[1], "--compareNumberOfPixelsTolerance") == 0 )
        {
        numberOfPixelsTolerance = atoi(av[2]);
        av += 2;
        ac -= 2;
        }
      else if( ac > 2 && strcmp(av[1], "--compareRadiusTolerance") == 0 )
        {
        radiusTolerance = atoi(av[2]);
        av += 2;
        ac -= 2;
        }
      else if( ac > 2 && strcmp(av[1], "--compareIntensityTolerance") == 0 )
        {
        intensityTolerance = atof(av[2]);
        av += 2;
        ac -= 2;
        }
      else if (strcmp(av[1], "--expectFail") == 0)
        {
        expectFail = true;
        av += 1;
        ac -= 1;
        }
      else
        {
        testToRun = av[1];
        }
      }
    }
  std::map<std::string, MainFuncPointer>::iterator j = StringToTestFunctionMap.find(testToRun);
  if( j != StringToTestFunctionMap.end() )
    {
    MainFuncPointer f = j->second;
    int             result;
    try
      {
      // Invoke the test's "main" function.
      result = ( *f )( ac - 1, av + 1 );
      // Make a list of possible baselines
      for( int i = 0; i < static_cast<int>( compareList.size() ); i++ )
        {
        char *                               baselineFilename = compareList[i].first;
        char *                               testFilename = compareList[i].second;
        std::map<std::string, int>           baselines = RegressionTestBaselines(baselineFilename);
        std::map<std::string, int>::iterator baseline = baselines.begin();
        std::string                          bestBaseline;
        int                                  bestBaselineStatus = itk::NumericTraits<int>::max();
        while( baseline != baselines.end() )
          {
          baseline->second = RegressionTestImage(testFilename,
                                                 ( baseline->first ).c_str(),
                                                 0,
                                                 intensityTolerance,
                                                 numberOfPixelsTolerance,
                                                 radiusTolerance);
          if( baseline->second < bestBaselineStatus )
            {
            bestBaseline = baseline->first;
            bestBaselineStatus = baseline->second;
            }
          if( baseline->second == 0 )
            {
            break;
            }
          ++baseline;
          }

        // if the best we can do still has errors, generate the error images
        if( bestBaselineStatus && !expectFail)
          {
          RegressionTestImage(testFilename,
                              bestBaseline.c_str(),
                              1,
                              intensityTolerance,
                              numberOfPixelsTolerance,
                              radiusTolerance);
          }

        // output the matching baseline
        std::cout << "<DartMeasurement name=\"BaselineImageName\" type=\"text/string\">";
        std::cout << itksys::SystemTools::GetFilenameName(bestBaseline);
        std::cout << "</DartMeasurement>" << std::endl;

        result += bestBaselineStatus;
        }
      }
    catch( const itk::ExceptionObject & e )
      {
      std::cerr << "ITK test driver caught an ITK exception:\n";
      e.Print(std::cerr);
      result = -1;
      }
    catch( const std::exception & e )
      {
      std::cerr << "ITK test driver caught an exception:\n";
      std::cerr << e.what() << "\n";
      result = -1;
      }
    catch( ... )
      {
      std::cerr << "ITK test driver caught an unknown exception!!!\n";
      result = -1;
      }
    if (!expectFail) {
      return result;
      }
    else {
      return result > 0 ? 0 : 1;
      }
    }
  PrintAvailableTests();
  std::cerr << "Failed: " << testToRun << ": No test registered with name " << testToRun << "\n";
  return -1;
}

// Regression Testing Code

int RegressionTestImage(const char *testImageFilename,
                        const char *baselineImageFilename,
                        int reportErrors,
                        double intensityTolerance,
                        ::itk::SizeValueType numberOfPixelsTolerance,
                        unsigned int radiusTolerance)
{
  // Use the factory mechanism to read the test and baseline files and convert
  // them to double
  typedef itk::Image<double, ITK_TEST_DIMENSION_MAX>        ImageType;
  typedef itk::VectorImage<double, ITK_TEST_DIMENSION_MAX>  VectorImageType;
  typedef itk::Image<unsigned char, ITK_TEST_DIMENSION_MAX> OutputType;
  typedef itk::Image<unsigned char, 2>                      DiffOutputType;
  typedef itk::ImageFileReader<VectorImageType>             ReaderType;

  // Read the baseline file
  ReaderType::Pointer baselineReader = ReaderType::New();
  baselineReader->SetFileName(baselineImageFilename);
  try
    {
    baselineReader->UpdateLargestPossibleRegion();
    }
  catch( itk::ExceptionObject & e )
    {
    std::cerr << "Exception detected while reading " << baselineImageFilename << " : "  << e.GetDescription();
    return 1000;
    }

  // Read the file generated by the test
  ReaderType::Pointer testReader = ReaderType::New();
  testReader->SetFileName(testImageFilename);
  try
    {
    testReader->UpdateLargestPossibleRegion();
    }
  catch( itk::ExceptionObject & e )
    {
    std::cerr << "Exception detected while reading " << testImageFilename << " : "  << e.GetDescription() << std::endl;
    return 1000;
    }

  VectorImageType::Pointer baslineVectorImage = baselineReader->GetOutput();
  VectorImageType::Pointer testVectorImage = testReader->GetOutput();

  // The sizes of the baseline and test image must match
  VectorImageType::SizeType baselineSize = baslineVectorImage->GetLargestPossibleRegion().GetSize();
  VectorImageType::SizeType testSize = testVectorImage->GetLargestPossibleRegion().GetSize();
  if( baselineSize != testSize )
    {
    std::cerr << "The size of the Baseline image and Test image do not match!" << std::endl;
    std::cerr << "Baseline image: " << baselineImageFilename
              << " has size " << baselineSize << std::endl;
    std::cerr << "Test image:     " << testImageFilename
              << " has size " << testSize << std::endl;
    return 1;
    }

  const unsigned int baselineNumberOfPixelComponents = baslineVectorImage->GetNumberOfComponentsPerPixel();
  const unsigned int testNumberOfPixelComponents = testVectorImage->GetNumberOfComponentsPerPixel();
  if (baselineNumberOfPixelComponents != testNumberOfPixelComponents)
  {
    std::cerr << "The number of components per pixel of the Baseline image and Test image do not match!" << std::endl;
    std::cerr << "Baseline image: " << baselineImageFilename
      << " has number of components " << baselineNumberOfPixelComponents << std::endl;
    std::cerr << "Test image:     " << testImageFilename
      << " has number of components " << testNumberOfPixelComponents << std::endl;
    return 1;
  }

  // Setup the filter to select indiviual vector image components
  typedef itk::VectorIndexSelectionCastImageFilter<VectorImageType, ImageType> IndexSelectionType;
  IndexSelectionType::Pointer baselineIndexSelectionFilter = IndexSelectionType::New();
  baselineIndexSelectionFilter->SetInput(baslineVectorImage);
  IndexSelectionType::Pointer testIndexSelectionFilter = IndexSelectionType::New();
  testIndexSelectionFilter->SetInput(testVectorImage);

  // Basic setup of filter to compare two scalar images
  typedef itk::Testing::ComparisonImageFilter<ImageType, ImageType> DiffType;
  DiffType::Pointer diff = DiffType::New();
  diff->SetDifferenceThreshold(intensityTolerance);
  diff->SetToleranceRadius(radiusTolerance);
  
  // Compare images going through each vector component and stopping as soon as the differences get too large.
  itk::SizeValueType status = 0;
  for (unsigned int idx = 0; (idx < baselineNumberOfPixelComponents) && (status <= numberOfPixelsTolerance); ++idx)
  {
    baselineIndexSelectionFilter->SetIndex(idx);
    testIndexSelectionFilter->SetIndex(idx);

    diff->SetValidInput(baselineIndexSelectionFilter->GetOutput());
    diff->SetTestInput(testIndexSelectionFilter->GetOutput());
    diff->UpdateLargestPossibleRegion();

    status += diff->GetNumberOfPixelsWithDifferences();
  }

  // if there are discrepencies, create an diff image
  if( ( status > numberOfPixelsTolerance ) && reportErrors )
    {
    typedef itk::RescaleIntensityImageFilter<ImageType, OutputType> RescaleType;
    typedef itk::ExtractImageFilter<OutputType, DiffOutputType>     ExtractType;
    typedef itk::ImageFileWriter<DiffOutputType>                    WriterType;
    typedef itk::ImageRegion<ITK_TEST_DIMENSION_MAX>                RegionType;
    OutputType::SizeType size; size.Fill(0);

    RescaleType::Pointer rescale = RescaleType::New();
    rescale->SetOutputMinimum( itk::NumericTraits<unsigned char>::NonpositiveMin() );
    rescale->SetOutputMaximum( itk::NumericTraits<unsigned char>::max() );
    rescale->SetInput( diff->GetOutput() );
    rescale->UpdateLargestPossibleRegion();
    size = rescale->GetOutput()->GetLargestPossibleRegion().GetSize();

    // Get the center slice of the image,  In 3D, the first slice
    // is often a black slice with little debugging information.
    OutputType::IndexType index; index.Fill(0);
    for( unsigned int i = 2; i < ITK_TEST_DIMENSION_MAX; i++ )
      {
      index[i] = size[i] / 2; // NOTE: Integer Divide used to get approximately
                              // the center slice
      size[i] = 0;
      }

    RegionType region;
    region.SetIndex(index);

    region.SetSize(size);

    ExtractType::Pointer extract = ExtractType::New();
    extract->SetDirectionCollapseToSubmatrix();
    extract->SetInput( rescale->GetOutput() );
    extract->SetExtractionRegion(region);

    WriterType::Pointer writer = WriterType::New();
    writer->SetInput( extract->GetOutput() );

    std::cout << "<DartMeasurement name=\"ImageError\" type=\"numeric/double\">";
    std::cout << status;
    std::cout <<  "</DartMeasurement>" << std::endl;

    std::ostringstream diffName;
    diffName << testImageFilename << ".diff.png";
    try
      {
      rescale->SetInput( diff->GetOutput() );
      rescale->Update();
      }
    catch( const std::exception & e )
      {
      std::cerr << "Error during rescale of " << diffName.str() << std::endl;
      std::cerr << e.what() << "\n";
      }
    catch( ... )
      {
      std::cerr << "Error during rescale of " << diffName.str() << std::endl;
      }
    writer->SetFileName( diffName.str().c_str() );
    try
      {
      writer->Update();
      }
    catch( const std::exception & e )
      {
      std::cerr << "Error during write of " << diffName.str() << std::endl;
      std::cerr << e.what() << "\n";
      }
    catch( ... )
      {
      std::cerr << "Error during write of " << diffName.str() << std::endl;
      }

    std::cout << "<DartMeasurementFile name=\"DifferenceImage\" type=\"image/png\">";
    std::cout << diffName.str();
    std::cout << "</DartMeasurementFile>" << std::endl;

    std::ostringstream baseName;
    baseName << testImageFilename << ".base.png";
    try
      {
      rescale->SetInput(baselineIndexSelectionFilter->GetOutput());
      rescale->Update();
      }
    catch( const std::exception & e )
      {
      std::cerr << "Error during rescale of " << baseName.str() << std::endl;
      std::cerr << e.what() << "\n";
      }
    catch( ... )
      {
      std::cerr << "Error during rescale of " << baseName.str() << std::endl;
      }
    try
      {
      writer->SetFileName( baseName.str().c_str() );
      writer->Update();
      }
    catch( const std::exception & e )
      {
      std::cerr << "Error during write of " << baseName.str() << std::endl;
      std::cerr << e.what() << "\n";
      }
    catch( ... )
      {
      std::cerr << "Error during write of " << baseName.str() << std::endl;
      }

    std::cout << "<DartMeasurementFile name=\"BaselineImage\" type=\"image/png\">";
    std::cout << baseName.str();
    std::cout << "</DartMeasurementFile>" << std::endl;

    std::ostringstream testName;
    testName << testImageFilename << ".test.png";
    try
      {
      rescale->SetInput(testIndexSelectionFilter->GetOutput());
      rescale->Update();
      }
    catch( const std::exception & e )
      {
      std::cerr << "Error during rescale of " << testName.str() << std::endl;
      std::cerr << e.what() << "\n";
      }
    catch( ... )
      {
      std::cerr << "Error during rescale of " << testName.str() << std::endl;
      }
    try
      {
      writer->SetFileName( testName.str().c_str() );
      writer->Update();
      }
    catch( const std::exception & e )
      {
      std::cerr << "Error during write of " << testName.str() << std::endl;
      std::cerr << e.what() << "\n";
      }
    catch( ... )
      {
      std::cerr << "Error during write of " << testName.str() << std::endl;
      }

    std::cout << "<DartMeasurementFile name=\"TestImage\" type=\"image/png\">";
    std::cout << testName.str();
    std::cout << "</DartMeasurementFile>" << std::endl;
    }
  return ( status > numberOfPixelsTolerance ) ? 1 : 0;
}

//
// Generate all of the possible baselines
// The possible baselines are generated fromn the baselineFilename using the
// following algorithm:
// 1) strip the suffix
// 2) append a digit .x
// 3) append the original suffix.
// It the file exists, increment x and continue
//
std::map<std::string, int> RegressionTestBaselines(char *baselineFilename)
{
  std::map<std::string, int> baselines;

  baselines[std::string(baselineFilename)] = 0;

  std::string originalBaseline(baselineFilename);

  int                    x = 0;
  std::string::size_type suffixPos = originalBaseline.rfind(".");
  std::string            suffix;
  if( suffixPos != std::string::npos )
    {
    suffix = originalBaseline.substr( suffixPos, originalBaseline.length() );
    originalBaseline.erase( suffixPos, originalBaseline.length() );
    }
  while( ++x )
    {
    std::ostringstream filename;
    filename << originalBaseline << "." << x << suffix;
    std::ifstream filestream( filename.str().c_str() );
    if( !filestream )
      {
      break;
      }
    baselines[filename.str()] = 0;
    filestream.close();
    }

  return baselines;
}

#endif
