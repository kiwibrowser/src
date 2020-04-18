// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "ios/showcase/manual_fill/password_picker_view_controller.h"

#import "ios/showcase/manual_fill/keyboard_proto_view_controller.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

namespace {

// The width for the cells in this collection view.
static CGFloat CellWitdth = 200.0;

// The height for the cells in this collection view.
static CGFloat CellHeight = 60.0;

// The reuse identifier for UICollectionViewCells in this collection.
static NSString* CellReuseId = @"CellReuseId";

}  // namespace

@interface PasswordPickerViewController ()

// The content to display in the collection view.
@property(nonatomic) NSArray<NSString*>* content;

// The delegate in charge of using the content selected by the user.
@property(nonatomic, readonly, weak) id<ManualFillContentDelegate> delegate;

@end

@implementation PasswordPickerViewController

@synthesize content = _content;
@synthesize delegate = _delegate;

- (instancetype)initWithDelegate:(id<ManualFillContentDelegate>)delegate {
  _delegate = delegate;
  UICollectionViewFlowLayout* flowLayout =
      [[UICollectionViewFlowLayout alloc] init];
  flowLayout.minimumInteritemSpacing = 0;
  flowLayout.minimumLineSpacing = 1;
  flowLayout.scrollDirection = UICollectionViewScrollDirectionVertical;
  flowLayout.estimatedItemSize = CGSizeMake(CellWitdth, CellHeight);
  return [super initWithCollectionViewLayout:flowLayout];
}

- (void)viewDidLoad {
  [super viewDidLoad];

  // TODO:(javierrobles) abstract this color. It was taken arbitrarly.
  self.collectionView.backgroundColor = [UIColor colorWithRed:250.0 / 255.0
                                                        green:250.0 / 255.0
                                                         blue:250.0 / 255.0
                                                        alpha:1.0];

  [self.collectionView registerClass:[UICollectionViewCell class]
          forCellWithReuseIdentifier:CellReuseId];

  if (@available(iOS 11.0, *)) {
    self.collectionView.contentInsetAdjustmentBehavior =
        UIScrollViewContentInsetAdjustmentAlways;
  }

  self.content = @[ @"Username", @"********" ];
}

- (CGSize)preferredContentSize {
  return CGSizeMake(250, 144);
}

#pragma mark - UICollectionViewDataSource

- (NSInteger)numberOfSectionsInCollectionView:
    (UICollectionView*)collectionView {
  return 1;
}

- (NSInteger)collectionView:(UICollectionView*)collectionView
     numberOfItemsInSection:(NSInteger)section {
  return [self.content count];
}

- (UICollectionViewCell*)collectionView:(UICollectionView*)collectionView
                 cellForItemAtIndexPath:(NSIndexPath*)indexPath {
  // TODO:(javierrobles) create a custom cell supporting a label.
  UICollectionViewCell* cell =
      [collectionView dequeueReusableCellWithReuseIdentifier:CellReuseId
                                                forIndexPath:indexPath];

  UILabel* titleLabel = [[UILabel alloc]
      initWithFrame:CGRectMake(0.0, 0.0, CellWitdth, CellHeight)];
  titleLabel.text = _content[indexPath.item];
  [cell.contentView addSubview:titleLabel];
  return cell;
}

#pragma mark - UICollectionViewDelegateFlowLayout

- (UIEdgeInsets)collectionView:(UICollectionView*)collectionView
                        layout:(UICollectionViewLayout*)collectionViewLayout
        insetForSectionAtIndex:(NSInteger)section {
  CGFloat horizontalInset =
      (collectionView.bounds.size.width - CellWitdth) / 2.0;
  return UIEdgeInsetsMake(0.0, horizontalInset, 0.0, horizontalInset);
}

- (void)collectionView:(UICollectionView*)collectionView
    didSelectItemAtIndexPath:(NSIndexPath*)indexPath {
  UICollectionViewCell* cell =
      [collectionView cellForItemAtIndexPath:indexPath];
  UIView* firstSubview = cell.contentView.subviews.firstObject;
  if ([firstSubview isKindOfClass:[UILabel class]]) {
    [self.delegate userDidPickContent:((UILabel*)firstSubview).text];
  }
}

@end
