//
//  NSBezierPath+MCAdditions.h
//
//  Created by Sean Patrick O'Brien on 4/1/08.
//  Copyright 2008 MolokoCacao. All rights reserved.
//

#ifndef THIRD_PARTY_MOLOKOCACAO_NSBEZIERPATH_MCADDITIONS_H_
#define THIRD_PARTY_MOLOKOCACAO_NSBEZIERPATH_MCADDITIONS_H_

#import <Cocoa/Cocoa.h>

@interface NSBezierPath (MCAdditions)

+ (NSBezierPath*)bezierPathWithCGPath:(CGPathRef)pathRef;

- (NSBezierPath*)pathWithStrokeWidth:(CGFloat)strokeWidth;

- (void)fillWithInnerShadow:(NSShadow *)shadow;
- (void)drawBlurWithColor:(NSColor*)color radius:(CGFloat)radius;

- (void)strokeInside;
- (void)strokeInsideWithinRect:(NSRect)clipRect;

@end

#endif  // THIRD_PARTY_MOLOKOCACAO_NSBEZIERPATH_MCADDITIONS_H_
