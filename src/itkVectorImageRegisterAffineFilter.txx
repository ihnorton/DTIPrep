/*=========================================================================

 Program:   GTRACT (Guided Tensor Restore Anatomical Connectivity Tractography)
 Module:    $RCSfile: itkVectorImageRegisterAffineFilter.txx,v $
 Language:  C++
 Date:      $Date: 2009-09-08 13:47:35 $
 Version:   $Revision: 1.1 $

   Copyright (c) University of Iowa Department of Radiology. All rights reserved.
   See GTRACT-Copyright.txt or http://mri.radiology.uiowa.edu/copyright/GTRACT-Copyright.txt 
   for details.
 
      This software is distributed WITHOUT ANY WARRANTY; without even 
      the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR 
      PURPOSE.  See the above copyright notices for more information.

=========================================================================*/

#include <itkImageRegionIterator.h>
#include <itkImageRegionConstIterator.h>
#include <itkIOCommon.h>
#include <itkImage.h>
#include <itkVectorImage.h>
#include <itkCastImageFilter.h>
#include <itkConstantPadImageFilter.h>
#include <itkExtractImageFilter.h>
#include <itkMetaDataObject.h>
#include <itkProgressAccumulator.h>

#include "itkVectorImageRegisterAffineFilter.h"

#include <iostream>


namespace itk
{

template <class TInputImage, class TOutputImage>
VectorImageRegisterAffineFilter<TInputImage, TOutputImage>
::VectorImageRegisterAffineFilter()
{
  m_NumberOfSpatialSamples = 100000;
  m_NumberOfIterations = 500;
  m_TranslationScale = 1000.0;
  m_MaximumStepLength = 0.2;
  m_MinimumStepLength = 0.0001;
  m_RelaxationFactor = 0.5;
  m_OutputParameterFile = "";
}


template <class TInputImage, class TOutputImage>
void VectorImageRegisterAffineFilter<TInputImage, TOutputImage>
::GenerateData ( )
{

  itkDebugMacro("VectorImageRegisterAffineFilter::GenerateData()");
  
  // Create a process accumulator for tracking the progress of this minipipeline
  typename ProgressAccumulator::Pointer progress = ProgressAccumulator::New();
  progress->SetMiniPipelineFilter(this);
  
  // No need to allocate the output since the minipipeline does it
  // this->AllocateOutputs();
  

  MetricTypePointer         metric        = MetricType::New();
  OptimizerTypePointer      optimizer     = OptimizerType::New();
  InterpolatorTypePointer   interpolator  = InterpolatorType::New();
  RegistrationTypePointer   registration  = RegistrationType::New();
  
  /* Allocate Output Image*/
  m_Output = OutputImageType::New( );
  m_Output->SetRegions( this->GetInput()->GetLargestPossibleRegion() );
  m_Output->SetSpacing( this->GetInput()->GetSpacing() );
  m_Output->SetOrigin( this->GetInput()->GetOrigin() );
  m_Output->SetDirection( this->GetInput()->GetDirection() );
  m_Output->SetVectorLength( this->GetInput()->GetVectorLength() );
  m_Output->SetMetaDataDictionary( this->GetInput()->GetMetaDataDictionary() );
  m_Output->Allocate();
 
  /* Create the Vector Index Extraction / Cast Filter */
  VectorIndexFilterPointer extractImageFilter = VectorIndexFilterType::New();
  extractImageFilter->SetInput( this->GetInput() );
  
  /* Create the Transform */
  TransformType::Pointer  transform = TransformType::New();
  TransformInitializerTypePointer initializer = TransformInitializerType::New();
  
  /* Create the Image Resampling Filter */
  ResampleFilterTypePointer resampler = ResampleFilterType::New();
  resampler->SetSize(    m_FixedImage->GetLargestPossibleRegion().GetSize() );
  resampler->SetOutputOrigin(  m_FixedImage->GetOrigin() );
  resampler->SetOutputSpacing( m_FixedImage->GetSpacing() );
  resampler->SetOutputDirection( m_FixedImage->GetDirection() );
  resampler->SetDefaultPixelValue( 0 );
  
  /* Create the Cast Image Filter */
  CastFilterTypePointer castImageFilter = CastFilterType::New( );
  
  /* Allocate the Writer - if requested */
  typedef itk::TransformFileWriter  TransformWriterType;
  TransformWriterType::Pointer    transformWriter = NULL;
  if (m_OutputParameterFile.length() != 0)
    {
    transformWriter =  TransformWriterType::New();
    transformWriter->SetFileName( m_OutputParameterFile.c_str() );
    }
      

  for (int i=0;i<static_cast<int>(this->GetInput()->GetVectorLength());i++)
    {
    itkDebugMacro("\tRegister Volume: " << i);

    extractImageFilter->SetIndex( i );
    extractImageFilter->Update( );
    
    /*** Set up the Registration ***/
    metric->SetNumberOfSpatialSamples( m_NumberOfSpatialSamples );
    registration->SetMetric(        metric        );
    registration->SetOptimizer(     optimizer     );
    registration->SetInterpolator(  interpolator  );
    registration->SetTransform( transform );

    

    /* Do we need this ???
    itk::Point<double, 3> zeroOrigin;
    zeroOrigin.GetVnlVector().fill(0.0);
    extractImageFilter->GetOutput()->SetOrigin(zeroOrigin);
    */
        
    registration->SetFixedImage(    m_FixedImage    );
    registration->SetMovingImage(   extractImageFilter->GetOutput()   );
    registration->SetFixedImageRegion( m_FixedImage->GetBufferedRegion() );

    initializer->SetTransform(   transform );
    initializer->SetFixedImage(  m_FixedImage );
    initializer->SetMovingImage( extractImageFilter->GetOutput() );
    initializer->MomentsOn();
    initializer->InitializeTransform();

    registration->SetInitialTransformParameters( transform->GetParameters() );

    const double translationScale = 1.0 / m_TranslationScale;

    OptimizerScalesType optimizerScales( transform->GetNumberOfParameters() );
    optimizerScales[0] = 1.0;
    optimizerScales[1] = 1.0;
    optimizerScales[2] = 1.0;
    optimizerScales[3] = 1.0;
    optimizerScales[4] = 1.0;
    optimizerScales[5] = 1.0;
    optimizerScales[6] = 1.0;
    optimizerScales[7] = 1.0;
    optimizerScales[8] = 1.0;
    optimizerScales[9] = translationScale;
    optimizerScales[10] = translationScale;
    optimizerScales[11] = translationScale;
    optimizer->SetScales( optimizerScales );

    optimizer->SetMaximumStepLength( m_MaximumStepLength );
    optimizer->SetMinimumStepLength( m_MinimumStepLength );
    optimizer->SetRelaxationFactor( m_RelaxationFactor );
    optimizer->SetNumberOfIterations( m_NumberOfIterations );
    
    registration->StartRegistration();
        

    OptimizerParameterType finalParameters = registration->GetLastTransformParameters();

#if 0
    const double rotatorXX              = finalParameters[0];
    const double rotatorXY              = finalParameters[1];
    const double rotatorXZ              = finalParameters[2];
    const double rotatorYX              = finalParameters[3];
    const double rotatorYY              = finalParameters[4];
    const double rotatorYZ              = finalParameters[5];
    const double rotatorZX              = finalParameters[6];
    const double rotatorZY              = finalParameters[7];
    const double rotatorZZ              = finalParameters[8];
    const double finalTranslationX      = finalParameters[9];
    const double finalTranslationY     = finalParameters[10];
    const double finalTranslationZ     = finalParameters[11];
    const unsigned int numberOfIterations = optimizer->GetCurrentIteration();
    const double bestValue = optimizer->GetValue();

    itkDebugMacro("\tResult = ");
    itkDebugMacro("\t\trotator XX      = " << rotatorXX);
    itkDebugMacro("\t\trotator XY      = " << rotatorXY);
    itkDebugMacro("\t\trotator XZ      = " << rotatorXZ);
    itkDebugMacro("\t\trotator YX      = " << rotatorYX);
    itkDebugMacro("\t\trotator YY      = " << rotatorYY);
    itkDebugMacro("\t\trotator YZ      = " << rotatorYZ);
    itkDebugMacro("\t\trotator ZX      = " << rotatorZX);
    itkDebugMacro("\t\trotator ZY      = " << rotatorZY);
    itkDebugMacro("\t\trotator ZZ      = " << rotatorZZ);
    itkDebugMacro("\t\tTranslation X = " << finalTranslationX);
    itkDebugMacro("\t\tTranslation Y = " << finalTranslationY);
    itkDebugMacro("\t\tTranslation Z = " << finalTranslationZ);
    itkDebugMacro("\t\tIterations    = " << numberOfIterations);
    itkDebugMacro("\t\tMetric value  = " << bestValue);
#endif


    transform->SetParameters( finalParameters );

    /* This step can probably be removed */
    TransformTypePointer finalTransform = TransformType::New();
    finalTransform->SetCenter( transform->GetCenter() );
    finalTransform->SetParameters( transform->GetParameters() );
    
    /* Add Transform To Writer */
    if (m_OutputParameterFile.length() != 0)
      {
      transformWriter->AddTransform( finalTransform );
      }


    /* Resample the Image */
    resampler->SetTransform( finalTransform );
    resampler->SetInput( extractImageFilter->GetOutput() );
    
    /* Cast the Image */
    castImageFilter->SetInput( resampler->GetOutput( ) );
    castImageFilter->Update( );
    
    
    /* Insert the Registered Vector Index Image into the Output Vector Image */
    typedef ImageRegionConstIterator<FixedImageType>  ConstIteratorType; 
    typedef ImageRegionIterator<OutputImageType>      IteratorType;
    
    ConstIteratorType it( castImageFilter->GetOutput(), castImageFilter->GetOutput()->GetRequestedRegion() );
    IteratorType ot( m_Output, m_Output->GetRequestedRegion() );
    OutputImagePixelType vectorImagePixel;
    
    for (ot.GoToBegin(),it.GoToBegin(); !ot.IsAtEnd(); ++ot, ++it)
      {
      vectorImagePixel = ot.Get( );
      vectorImagePixel[i] = it.Value();
      ot.Set( vectorImagePixel );
      }
    
	// Update Gradient directions 
    vnl_vector<double> curGradientDirection(3);
    char tmpStr[64];
    sprintf(tmpStr, "DWMRI_gradient_%04d", i);
    std::string KeyString(tmpStr);
    std::string NrrdValue;

    /* Rebinding inputMetaDataDictionary is necessary until 
       itk::ExposeMetaData accepts a const MetaDataDictionary. */
    itk::MetaDataDictionary inputMetaDataDictionary = this->GetInput()->GetMetaDataDictionary();
    itk::ExposeMetaData<std::string>(inputMetaDataDictionary, KeyString, NrrdValue);
    /* %lf is 'long float', i.e., double. */
    sscanf(NrrdValue.c_str()," %lf %lf %lf", &curGradientDirection[0], &curGradientDirection[1], &curGradientDirection[2]);
	//std::cout << "Matrix : " << finalTransform->GetMatrix().GetVnlMatrix() << std::endl;
	
	Matrix3D   NonOrthog = finalTransform->GetMatrix();
    Matrix3D   Orthog( this->orthogonalize(NonOrthog) );

	//std::cout << "Matrix : " << Orthog.GetVnlMatrix() << std::endl;
    curGradientDirection = Orthog.GetVnlMatrix() * curGradientDirection;
    sprintf(tmpStr," %18.15lf %18.15lf %18.15lf", curGradientDirection[0], curGradientDirection[1], curGradientDirection[2]);
    NrrdValue = tmpStr;
    itk::EncapsulateMetaData<std::string>(m_Output->GetMetaDataDictionary(), KeyString, NrrdValue);
	
    /* Update Progress */
    this->UpdateProgress((float)(i+1)/(float)(this->GetInput()->GetVectorLength()));      
    }
    
  if (m_OutputParameterFile.length() != 0)
    {
    transformWriter->Update( );
    }
      
}  

template <class TInputImage, class TOutputImage>
itk::Matrix<double, 3, 3> VectorImageRegisterAffineFilter<TInputImage, TOutputImage>
::orthogonalize( Matrix3D rotator)
{
  vnl_svd<double> decomposition(
    rotator.GetVnlMatrix(),
    -1E-6);
  vnl_diag_matrix<vnl_svd<double>::singval_t> Winverse( decomposition.Winverse() );

  vnl_matrix<double> W(3, 3);
  W.fill( double(0) );

  for ( unsigned int i = 0; i < 3; ++i )
    {
    if ( decomposition.Winverse() (i, i) != 0.0 )
      {
      W(i, i) = 1.0;
      }
    }

  vnl_matrix<double> result(
    decomposition.U() * W * decomposition.V().conjugate_transpose() );

  //    std::cout << " svd Orthonormalized Rotation: " << std::endl
  //      << result << std::endl;
  Matrix3D Orthog;
  Orthog.operator=(result);

  return Orthog;
}



}// end namespace itk

