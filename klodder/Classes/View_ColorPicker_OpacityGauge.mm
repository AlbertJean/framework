#import <QuartzCore/QuartzCore.h>
#import "Calc.h"
#import "ExceptionLoggerObjC.h"
#import "Log.h"
#import "View_ColorPicker.h"
#import "View_ColorPicker_OpacityGauge.h"
#import "View_ColorPicker_OpacityIndicator.h"

@implementation View_ColorPicker_OpacityGauge

@synthesize value;

-(id)initWithFrame:(CGRect)frame height:(float)_height delegate:(id<PickerDelegate>)_delegate
{
	HandleExceptionObjcBegin();
	
    if ((self = [super initWithFrame:frame]))
	{
		[self setMultipleTouchEnabled:FALSE];
		[self setOpaque:FALSE];
		[self setBackgroundColor:[UIColor clearColor]];
		
		height = _height;
		delegate = _delegate;
		
		indicator = [[View_ColorPicker_OpacityIndicator alloc] initWithDelegate:delegate];
		[self addSubview:indicator];

		[self setValue:delegate.colorPickerState->Opacity_get()];
    }
	
    return self;
	
	HandleExceptionObjcEnd(false);
	
	return nil;
}

-(void)sample:(Vec2F)location
{
	float _value = location[0] / self.frame.size.width;
	
	_value = Calc::Mid(_value, 0.0f, 1.0f);
	
	delegate.colorPickerState->Opacity_set(_value);
	[delegate colorChanged];
}

-(void)setValueDirect:(float)_value
{
	value = _value;
	
	[self setNeedsDisplay];
	[indicator setNeedsDisplay];
	
	float x = self.frame.size.width * value;
	float y = self.frame.size.height / 2.0f;
	
	[indicator.layer setPosition:CGPointMake(x, y)];
}

-(void)drawRect:(CGRect)rect 
{
	HandleExceptionObjcBegin();
	
	CGContextRef ctx = UIGraphicsGetCurrentContext();
	CGContextSaveGState(ctx);
	
	int y = (self.bounds.size.height - height) / 2.0f;

	CGContextClipToRect(ctx, CGRectMake(0, y, self.bounds.size.width, height));
	
	{
		Rgba color = delegate.colorPickerState->Color_get();
		CGColorSpaceRef colorSpace = CGColorSpaceCreateDeviceRGB();
		CGFloat components[] = { color.rgb[0], color.rgb[1], color.rgb[2], 0.0f, color.rgb[0], color.rgb[1], color.rgb[2], 1.0f };
		CGFloat locations[] = { 0.0f, 1.0f };
		CGGradientRef gradient = CGGradientCreateWithColorComponents(colorSpace, components, locations, 2);
		CGContextDrawLinearGradient(ctx, gradient, CGPointMake(0.0f, y), CGPointMake(self.frame.size.width, height), 0);
		CGGradientRelease(gradient);
		CGColorSpaceRelease(colorSpace);
	}
	{
		CGContextSetLineWidth(ctx, 3.0f);
		CGContextSetStrokeColorWithColor(ctx, [UIColor blackColor].CGColor);
		CGContextStrokeRect(ctx, CGRectMake(0.0f, y, self.frame.size.width, height));
	}
	
	CGContextRestoreGState(ctx);
	
	HandleExceptionObjcEnd(false);
}

-(void)touchesBegan:(NSSet *)touches withEvent:(UIEvent *)event
{
	HandleExceptionObjcBegin();
	
	CGPoint point = [[[touches allObjects] objectAtIndex:0] locationInView:self];
	
	Vec2F location(point.x, point.y);
	
	[self sample:location];
	
	HandleExceptionObjcEnd(false);
}

-(void)touchesMoved:(NSSet *)touches withEvent:(UIEvent *)event
{
	HandleExceptionObjcBegin();
	
	CGPoint point = [[[touches allObjects] objectAtIndex:0] locationInView:self];
	
	Vec2F location(point.x, point.y);
	
	[self sample:location];

	HandleExceptionObjcEnd(false);
}

-(void)dealloc
{
	HandleExceptionObjcBegin();
	
	LOG_DBG("dealloc: View_ColorPicker_OpacityGauge", 0);
	
    [super dealloc];
	
	HandleExceptionObjcEnd(false);
}

@end
