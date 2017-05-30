#include "vfxNodeImageCpuDownsample.h"
#include <xmmintrin.h>

// todo : test with really small input textures and maxSx/maxSy set

VfxNodeImageCpuDownsample::VfxNodeImageCpuDownsample()
	: VfxNodeBase()
	, buffers()
	, imageOutput()
{
	resizeSockets(kInput_COUNT, kOutput_COUNT);
	addInput(kInput_Image, kVfxPlugType_ImageCpu);
	addInput(kInput_DownsampleSize, kVfxPlugType_Int);
	addInput(kInput_DownsampleChannel, kVfxPlugType_Int);
	addInput(kInput_MaxSx, kVfxPlugType_Int);
	addInput(kInput_MaxSy, kVfxPlugType_Int);
	addOutput(kOutput_Image, kVfxPlugType_ImageCpu, &imageOutput);
}

VfxNodeImageCpuDownsample::~VfxNodeImageCpuDownsample()
{
	freeImage();
}

void VfxNodeImageCpuDownsample::tick(const float dt)
{
	const VfxImageCpu * image = getInputImageCpu(kInput_Image, nullptr);
	const DownsampleSize downsampleSize = (DownsampleSize)getInputInt(kInput_DownsampleSize, kDownsampleSize_2x2);
	const DownsampleChannel downsampleChannel = (DownsampleChannel)getInputInt(kInput_DownsampleChannel, kDownsampleChannel_All);
	int maxSx = std::max(0, getInputInt(kInput_MaxSx, 0));
	int maxSy = std::max(0, getInputInt(kInput_MaxSy, 0));

	if (image == nullptr || image->sx == 0 || image->sy == 0)
	{
		freeImage();
	}
	else
	{
		const int pixelSize = downsampleSize == kDownsampleSize_2x2 ? 2 : 4;
		const int numChannels = downsampleChannel == kDownsampleChannel_All ? image->numChannels : 1;
		
		if (maxSx == 0 && maxSy > 0)
			maxSx = image->sx;
		if (maxSy == 0 && maxSx > 0)
			maxSy = image->sy;
		
		if (image->sx != buffers.sx || image->sy != buffers.sy || numChannels != buffers.numChannels || maxSx != buffers.maxSx || maxSy != buffers.maxSy || pixelSize != buffers.pixelSize)
		{
			allocateImage(image->sx, image->sy, numChannels, maxSx, maxSy, pixelSize);
		}
		
		// the initial source image is just the input image ..
		
		VfxImageCpu srcImage = *image;
		
		// but with a twist .. we remap the channels here according to the channels input
		
		for (int i = 0; i < 4; ++i)
		{
			const int srcChannelIndex =
				downsampleChannel == kDownsampleChannel_All ? i
				: downsampleChannel == kDownsampleChannel_R ? 0
				: downsampleChannel == kDownsampleChannel_G ? 1
				: downsampleChannel == kDownsampleChannel_B ? 2
				: downsampleChannel == kDownsampleChannel_A ? 3
				: 0;
			
			const VfxImageCpu::Channel & srcChannel = image->channel[srcChannelIndex];
			
			srcImage.channel[i] = srcChannel;
		}
	
		if (maxSx > 0 || maxSy > 0)
		{
			int bufferIndex = 0;
			
			int downsampledSx = image->sx;
			int downsampledSy = image->sy;
			
			if (downsampledSx <= buffers.maxSx && downsampledSy <= buffers.maxSy)
			{
				// criteria are already met; just copy the data
				
				VfxImageCpu dstImage;
				
				if (numChannels == 1)
					dstImage.setDataR8(buffers.data1, downsampledSx, downsampledSy, 16, downsampledSx * numChannels);
				else if (numChannels == 4)
					dstImage.setDataRGBA8(buffers.data1, downsampledSx, downsampledSy, 16, downsampledSx * numChannels);
				else
					Assert(false);
				
				for (int i = 0; i < numChannels; ++i)
				{
					const VfxImageCpu::Channel & srcChannel = srcImage.channel[i];
					      VfxImageCpu::Channel & dstChannel = dstImage.channel[i];
					
					for (int y = 0; y < image->sy; ++y)
					{
						const uint8_t * __restrict srcItr = srcChannel.data + y * srcChannel.pitch;
						      uint8_t * __restrict dstItr = (uint8_t*)dstChannel.data + y * dstChannel.pitch;
						
						for (int x = 0; x < image->sx; ++x)
						{
							*dstItr = *srcItr;
							
							srcItr += srcChannel.stride;
							dstItr += dstChannel.stride;
						}
					}
				}
				
				srcImage = dstImage;
			}
			else
			{
				while (downsampledSx > buffers.maxSx || downsampledSy > buffers.maxSy)
				{
					downsampledSx = std::max(1, downsampledSx / pixelSize);
					downsampledSy = std::max(1, downsampledSy / pixelSize);
					
					uint8_t * data = bufferIndex == 0 ? buffers.data1 : buffers.data2;
					bufferIndex = (bufferIndex + 1) % 2;
					
					VfxImageCpu dstImage;
					
					if (numChannels == 1)
						dstImage.setDataR8(data, downsampledSx, downsampledSy, 16, downsampledSx * numChannels);
					else if (numChannels == 4)
						dstImage.setDataRGBA8(data, downsampledSx, downsampledSy, 16, downsampledSx * numChannels);
					else
						Assert(false);
					
					downsample(srcImage, dstImage, pixelSize, downsampleChannel);
					
					srcImage = dstImage;
				}
			}
			
			imageOutput = srcImage;
		}
		else
		{
			const int downsampledSx = std::max(1, image->sx / pixelSize);
			const int downsampledSy = std::max(1, image->sy / pixelSize);
			
			if (numChannels == 1)
				imageOutput.setDataR8(buffers.data1, downsampledSx, downsampledSy, 16, downsampledSx * numChannels);
			else if (numChannels == 4)
				imageOutput.setDataRGBA8(buffers.data1, downsampledSx, downsampledSy, 16, downsampledSx * numChannels);
			else
				Assert(false);
			
			downsample(srcImage, imageOutput, pixelSize, downsampleChannel);
		}
	}
}

void VfxNodeImageCpuDownsample::allocateImage(const int sx, const int sy, const int numChannels, const int maxSx, const int maxSy, const int pixelSize)
{
	freeImage();
	
	buffers.sx = sx;
	buffers.sy = sy;
	buffers.numChannels = numChannels;
	buffers.maxSx = maxSx;
	buffers.maxSy = maxSy;
	buffers.pixelSize = pixelSize;
	
	if (maxSx > 0 || maxSy > 0)
	{
		if (sx <= maxSx && sy <= maxSy)
		{
			// note : if the downsample criteria are already met by the input image, we need a space to store the
			//        entire image's contents. so our buffer needs to be full-size. ideally we could just reference
			//        the original channel data in our own output image, but unfortunately tick order is not
			//        guaranteed right now when nodes do not directly connect to the display node
			
			// todo : reference input image output data once typology is recursed in dependency order regardless
			//        of connectivity with display node
			
			Assert(buffers.data1 == nullptr);
			buffers.data1 = (uint8_t*)_mm_malloc(sx * sy * numChannels, 16);
		}
		else
		{
			int downsampledSx = sx;
			int downsampledSy = sy;
			
			//
			
			downsampledSx = std::max(1, downsampledSx / pixelSize);
			downsampledSy = std::max(1, downsampledSy / pixelSize);
			
			Assert(buffers.data1 == nullptr);
			buffers.data1 = (uint8_t*)_mm_malloc(downsampledSx * downsampledSy * numChannels, 16);
			
			//
			
			downsampledSx = std::max(1, downsampledSx / pixelSize);
			downsampledSy = std::max(1, downsampledSy / pixelSize);
			
			Assert(buffers.data2 == nullptr);
			buffers.data2 = (uint8_t*)_mm_malloc(downsampledSx * downsampledSy * numChannels, 16);
		}
	}
	else
	{
		const int downsampledSx = std::max(1, sx / pixelSize);
		const int downsampledSy = std::max(1, sy / pixelSize);
		
		Assert(buffers.data1 == nullptr);
		buffers.data1 = (uint8_t*)_mm_malloc(downsampledSx * downsampledSy * numChannels, 16);
	}
}

void VfxNodeImageCpuDownsample::freeImage()
{
	_mm_free(buffers.data1);
	buffers.data1 = nullptr;
	
	_mm_free(buffers.data2);
	buffers.data2 = nullptr;
	
	buffers = Buffers();
	
	imageOutput.reset();
}

void VfxNodeImageCpuDownsample::downsample(const VfxImageCpu & src, VfxImageCpu & dst, const int pixelSize, const DownsampleChannel downsampleChannel)
{
	if (pixelSize == 2)
	{
		const int downsampledSx = std::max(1, src.sx / pixelSize);
		const int downsampledSy = std::max(1, src.sy / pixelSize);
		
		for (int i = 0; i < dst.numChannels; ++i)
		{
			const VfxImageCpu::Channel & srcChannel = src.channel[i];
				  VfxImageCpu::Channel & dstChannel = dst.channel[i];
			
			for (int y = 0; y < downsampledSy; ++y)
			{
				const uint8_t * __restrict srcItr1 = srcChannel.data + (y * 2 + 0) * srcChannel.pitch;
				const uint8_t * __restrict srcItr2 = srcChannel.data + (y * 2 + 1) * srcChannel.pitch;
					  uint8_t * __restrict dstItr = (uint8_t*)dstChannel.data + y * dstChannel.pitch;
				
				for (int x = 0; x < downsampledSx; ++x)
				{
					int src1 = 0;
					int src2 = 0;
					
					for (int i = 0; i < 2; ++i) { src1 += *srcItr1; srcItr1 += srcChannel.stride; }
					for (int i = 0; i < 2; ++i) { src2 += *srcItr2; srcItr2 += srcChannel.stride; }
					
					int src = src1 + src2;
					
					src >>= 2;
					
					*dstItr = src;
					
					dstItr += dstChannel.stride;
				}
			}
		}
	}
	else if (pixelSize == 4)
	{
		const int downsampledSx = std::max(1, src.sx / pixelSize);
		const int downsampledSy = std::max(1, src.sy / pixelSize);
		
		for (int i = 0; i < dst.numChannels; ++i)
		{
			const VfxImageCpu::Channel & srcChannel = src.channel[i];
				  VfxImageCpu::Channel & dstChannel = dst.channel[i];
			
			for (int y = 0; y < downsampledSy; ++y)
			{
				const uint8_t * __restrict srcItr1 = srcChannel.data + (y * 4 + 0) * srcChannel.pitch;
				const uint8_t * __restrict srcItr2 = srcChannel.data + (y * 4 + 1) * srcChannel.pitch;
				const uint8_t * __restrict srcItr3 = srcChannel.data + (y * 4 + 2) * srcChannel.pitch;
				const uint8_t * __restrict srcItr4 = srcChannel.data + (y * 4 + 3) * srcChannel.pitch;
					  uint8_t * __restrict dstItr = (uint8_t*)dstChannel.data + y * dstChannel.pitch;
				
				for (int x = 0; x < downsampledSx; ++x)
				{
					int src1 = 0;
					int src2 = 0;
					int src3 = 0;
					int src4 = 0;
					
					for (int i = 0; i < 4; ++i) { src1 += *srcItr1; srcItr1 += srcChannel.stride; }
					for (int i = 0; i < 4; ++i) { src2 += *srcItr2; srcItr2 += srcChannel.stride; }
					for (int i = 0; i < 4; ++i) { src3 += *srcItr3; srcItr3 += srcChannel.stride; }
					for (int i = 0; i < 4; ++i) { src4 += *srcItr4; srcItr4 += srcChannel.stride; }
					
					int src = (src1 + src2) + (src3 + src4);
					
					src >>= 4;
					
					*dstItr = src;
					
					dstItr += dstChannel.stride;
				}
			}
		}
	}
	else
	{
		Assert(false);
	}
}
