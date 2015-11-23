#include <mbgl/util/image.hpp>

#import <ImageIO/ImageIO.h>

#if TARGET_OS_IPHONE
#import <MobileCoreServices/MobileCoreServices.h>
#else
#import <CoreServices/CoreServices.h>
#endif

namespace mbgl {

std::string encodePNG(const UnassociatedImage& src) {
    CGDataProviderRef provider = CGDataProviderCreateWithData(NULL, src.data.get(), src.size(), NULL);
    if (!provider) {
        return "";
    }

    CGColorSpaceRef colorSpace = CGColorSpaceCreateDeviceRGB();
    if (!colorSpace) {
        CGDataProviderRelease(provider);
        throw std::runtime_error("CGColorSpaceCreateDeviceRGB failed");
    }

    CGImageRef image = CGImageCreate(src.width,
                                     src.height,
                                     8, 32,
                                     4 * src.width,
                                     colorSpace,
                                     kCGBitmapByteOrderDefault | kCGImageAlphaLast,
                                     provider,
                                     NULL, false,
                                     kCGRenderingIntentDefault);
    if (!image) {
        CGColorSpaceRelease(colorSpace);
        CGDataProviderRelease(provider);
        throw std::runtime_error("CGImageCreate failed");
    }

    CFMutableDataRef data = CFDataCreateMutable(kCFAllocatorDefault, 0);
    if (!data) {
        CGImageRelease(image);
        CGColorSpaceRelease(colorSpace);
        CGDataProviderRelease(provider);
        throw std::runtime_error("CFDataCreateMutable failed");
    }

    CGImageDestinationRef imageDestination = CGImageDestinationCreateWithData(data, kUTTypePNG, 1, NULL);
    if (!imageDestination) {
        CFRelease(data);
        CGImageRelease(image);
        CGColorSpaceRelease(colorSpace);
        CGDataProviderRelease(provider);
        throw std::runtime_error("CGImageDestinationCreateWithData failed");
    }

    CGImageDestinationAddImage(imageDestination, image, NULL);
    CGImageDestinationFinalize(imageDestination);

    const std::string result {
        reinterpret_cast<const char *>(CFDataGetBytePtr(data)),
        static_cast<size_t>(CFDataGetLength(data))
    };

    CFRelease(imageDestination);
    CFRelease(data);
    CGImageRelease(image);
    CGColorSpaceRelease(colorSpace);
    CGDataProviderRelease(provider);

    return result;
}

UnassociatedImage decodeImage(const std::string& sourceData) {
    CFDataRef data = CFDataCreateWithBytesNoCopy(kCFAllocatorDefault,
                                                 reinterpret_cast<const unsigned char *>(sourceData.data()),
                                                 sourceData.size(),
                                                 kCFAllocatorNull);
    if (!data) {
        throw std::runtime_error("CFDataCreateWithBytesNoCopy failed");
    }

    CGImageSourceRef imageSource = CGImageSourceCreateWithData(data, NULL);
    if (!imageSource) {
        CFRelease(data);
        throw std::runtime_error("CGImageSourceCreateWithData failed");
    }

    CGImageRef image = CGImageSourceCreateImageAtIndex(imageSource, 0, NULL);
    if (!image) {
        CFRelease(imageSource);
        CFRelease(data);
        throw std::runtime_error("CGImageSourceCreateImageAtIndex failed");
    }

    CFDataRef dataCopy = CGDataProviderCopyData(CGImageGetDataProvider(image));

    size_t width = CGImageGetWidth(image);
    size_t height = CGImageGetHeight(image);
    CGBitmapInfo info = CGImageGetBitmapInfo(image);
    size_t bpc = CGImageGetBitsPerComponent(image);
    size_t bpp = CGImageGetBitsPerPixel(image);
    CGImageAlphaInfo alpha = CGImageAlphaInfo(info & kCGBitmapAlphaInfoMask);

    if (bpc != 8 ||
        (info & kCGBitmapFloatComponents) ||
        (info & kCGBitmapByteOrderMask) != kCGBitmapByteOrderDefault ||
        !((bpp == 32 && (alpha == kCGImageAlphaLast || alpha == kCGImageAlphaNoneSkipLast)) ||
          (bpp == 24 && alpha == kCGImageAlphaNone))) {
        CFRelease(dataCopy);
        CGImageRelease(image);
        CFRelease(imageSource);
        CFRelease(data);
        throw std::runtime_error("unsupported image format");
    }

    UnassociatedImage result { width, height };

    const uint8_t* src = CFDataGetBytePtr(dataCopy);
    const uint8_t* end = src + CFDataGetLength(dataCopy);
          uint8_t* dst = result.data.get();

    while (src < end) {
        dst[0] = src[0];
        dst[1] = src[1];
        dst[2] = src[2];
        dst[3] = alpha == kCGImageAlphaLast ? src[3] : 0xFF;
        src += bpp / bpc;
        dst += 4;
    }

    CFRelease(dataCopy);
    CGImageRelease(image);
    CFRelease(imageSource);
    CFRelease(data);

    return result;
}

}
