// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "ios/chrome/browser/ui/reading_list/reading_list_mediator.h"

#include <algorithm>

#import "base/mac/foundation_util.h"
#include "base/metrics/histogram_macros.h"
#include "base/strings/sys_string_conversions.h"
#include "components/favicon/core/large_icon_service.h"
#include "components/reading_list/core/reading_list_model.h"
#import "components/reading_list/ios/reading_list_model_bridge_observer.h"
#include "components/url_formatter/url_formatter.h"
#import "ios/chrome/browser/ui/favicon/favicon_attributes_provider.h"
#import "ios/chrome/browser/ui/favicon/favicon_view.h"
#import "ios/chrome/browser/ui/reading_list/reading_list_collection_view_item.h"
#import "ios/chrome/browser/ui/reading_list/reading_list_collection_view_item_accessibility_delegate.h"
#import "ios/chrome/browser/ui/reading_list/reading_list_data_sink.h"
#import "ios/chrome/browser/ui/reading_list/reading_list_utils.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

namespace {
bool EntrySorter(const ReadingListEntry* rhs, const ReadingListEntry* lhs) {
  return rhs->UpdateTime() > lhs->UpdateTime();
}
}  // namespace

@interface ReadingListMediator ()<ReadingListModelBridgeObserver> {
  std::unique_ptr<ReadingListModelBridge> _modelBridge;
  std::unique_ptr<ReadingListModel::ScopedReadingListBatchUpdate> _batchToken;
}

@property(nonatomic, assign) ReadingListModel* model;

@property(nonatomic, assign) BOOL shouldMonitorModel;

// Lazily instantiated.
@property(nonatomic, strong, readonly)
    FaviconAttributesProvider* attributesProvider;

@property(nonatomic, assign, readonly)
    favicon::LargeIconService* largeIconService;

@end

@implementation ReadingListMediator

@synthesize model = _model;
@synthesize dataSink = _dataSink;
@synthesize shouldMonitorModel = _shouldMonitorModel;
@synthesize attributesProvider = _attributesProvider;
@synthesize largeIconService = _largeIconService;

#pragma mark - Public

- (instancetype)initWithModel:(ReadingListModel*)model
             largeIconService:(favicon::LargeIconService*)largeIconService {
  self = [super init];
  if (self) {
    _model = model;
    _largeIconService = largeIconService;
    _shouldMonitorModel = YES;

    // This triggers the callback method. Should be created last.
    _modelBridge.reset(new ReadingListModelBridge(self, model));
  }
  return self;
}

- (const ReadingListEntry*)entryFromItem:(CollectionViewItem*)item {
  ReadingListCollectionViewItem* readingListItem =
      base::mac::ObjCCastStrict<ReadingListCollectionViewItem>(item);
  return self.model->GetEntryByURL(readingListItem.url);
}

- (void)markEntryRead:(const GURL&)URL {
  self.model->SetReadStatus(URL, true);
}

#pragma mark - ReadingListDataSource

- (BOOL)isEntryRead:(CollectionViewItem*)item {
  ReadingListCollectionViewItem* readingListItem =
      base::mac::ObjCCastStrict<ReadingListCollectionViewItem>(item);
  const ReadingListEntry* readingListEntry =
      self.model->GetEntryByURL(readingListItem.url);

  if (!readingListEntry) {
    return NO;
  }

  return readingListEntry->IsRead();
}

- (void)dataSinkWillBeDismissed {
  self.model->MarkAllSeen();
  // Reset data sink to prevent further model update notifications.
  self.dataSink = nil;
}

- (void)setReadStatus:(BOOL)read forItem:(CollectionViewItem*)item {
  ReadingListCollectionViewItem* readingListItem =
      base::mac::ObjCCastStrict<ReadingListCollectionViewItem>(item);
  self.model->SetReadStatus(readingListItem.url, read);
}

- (const ReadingListEntry*)entryWithURL:(const GURL&)URL {
  return self.model->GetEntryByURL(URL);
}

- (void)removeEntryFromItem:(CollectionViewItem*)item {
  ReadingListCollectionViewItem* readingListItem =
      base::mac::ObjCCastStrict<ReadingListCollectionViewItem>(item);
  [self logDeletionOfItem:readingListItem];
  self.model->RemoveEntryByURL(readingListItem.url);
}

- (void)fillReadItems:(NSMutableArray<CollectionViewItem*>*)readArray
          unreadItems:(NSMutableArray<CollectionViewItem*>*)unreadArray
         withDelegate:
             (id<ReadingListCollectionViewItemAccessibilityDelegate>)delegate {
  std::vector<const ReadingListEntry*> readEntries;
  std::vector<const ReadingListEntry*> unreadEntries;

  for (const auto& url : self.model->Keys()) {
    const ReadingListEntry* entry = self.model->GetEntryByURL(url);
    DCHECK(entry);
    if (entry->IsRead()) {
      readEntries.push_back(entry);
    } else {
      unreadEntries.push_back(entry);
    }
  }

  std::sort(readEntries.begin(), readEntries.end(), EntrySorter);
  std::sort(unreadEntries.begin(), unreadEntries.end(), EntrySorter);

  for (const ReadingListEntry* entry : readEntries) {
    [readArray addObject:[self cellItemForReadingListEntry:entry
                                              withDelegate:delegate]];
  }

  for (const ReadingListEntry* entry : unreadEntries) {
    [unreadArray addObject:[self cellItemForReadingListEntry:entry
                                                withDelegate:delegate]];
  }

  DCHECK(self.model->Keys().size() == [readArray count] + [unreadArray count]);
}

- (void)fetchFaviconForItem:(CollectionViewItem*)item {
  ReadingListCollectionViewItem* readingListItem =
      base::mac::ObjCCastStrict<ReadingListCollectionViewItem>(item);
  __weak ReadingListCollectionViewItem* weakItem = readingListItem;
  __weak ReadingListMediator* weakSelf = self;
  void (^completionBlock)(FaviconAttributes* attributes) =
      ^(FaviconAttributes* attributes) {
        ReadingListCollectionViewItem* strongItem = weakItem;
        ReadingListMediator* strongSelf = weakSelf;
        if (!strongSelf || !strongItem) {
          return;
        }

        strongItem.attributes = attributes;

        [strongSelf.dataSink itemHasChangedAfterDelay:strongItem];
      };

  [self.attributesProvider
      fetchFaviconAttributesForURL:readingListItem.faviconPageURL
                        completion:completionBlock];
}

- (void)beginBatchUpdates {
  self.shouldMonitorModel = NO;
  _batchToken = self.model->BeginBatchUpdates();
}

- (void)endBatchUpdates {
  _batchToken.reset();
  self.shouldMonitorModel = YES;
}

#pragma mark - Properties

- (FaviconAttributesProvider*)attributesProvider {
  if (_attributesProvider) {
    return _attributesProvider;
  }

  // Accept any favicon even the smallest ones (16x16) as it is better than the
  // fallback icon.
  // Pass 1 as minimum size to avoid empty favicons.
  _attributesProvider = [[FaviconAttributesProvider alloc]
      initWithFaviconSize:kFaviconPreferredSize
           minFaviconSize:1
         largeIconService:self.largeIconService];
  return _attributesProvider;
}

- (void)setDataSink:(id<ReadingListDataSink>)dataSink {
  _dataSink = dataSink;
  if (self.model->loaded()) {
    [dataSink dataSourceReady:self];
  }
}

- (BOOL)ready {
  return self.model->loaded();
}

- (BOOL)hasElements {
  return self.model->size() > 0;
}

- (BOOL)hasRead {
  return self.model->size() != self.model->unread_size();
}

#pragma mark - ReadingListModelBridgeObserver

- (void)readingListModelLoaded:(const ReadingListModel*)model {
  UMA_HISTOGRAM_COUNTS_1000("ReadingList.Unread.Number", model->unread_size());
  UMA_HISTOGRAM_COUNTS_1000("ReadingList.Read.Number",
                            model->size() - model->unread_size());
  [self.dataSink dataSourceReady:self];
}

- (void)readingListModelDidApplyChanges:(const ReadingListModel*)model {
  if (!self.shouldMonitorModel) {
    return;
  }

  // Ignore single element updates when the data source is doing batch updates.
  if (self.model->IsPerformingBatchUpdates()) {
    return;
  }

  if ([self hasDataSourceChanged])
    [self.dataSink dataSourceChanged];
}

- (void)readingListModelCompletedBatchUpdates:(const ReadingListModel*)model {
  if (!self.shouldMonitorModel) {
    return;
  }

  if ([self hasDataSourceChanged])
    [self.dataSink dataSourceChanged];
}

#pragma mark - Private

// Creates a ReadingListCollectionViewItem from a ReadingListEntry |entry|.
- (ReadingListCollectionViewItem*)
cellItemForReadingListEntry:(const ReadingListEntry*)entry
               withDelegate:
                   (id<ReadingListCollectionViewItemAccessibilityDelegate>)
                       delegate {
  const GURL& url = entry->URL();
  ReadingListCollectionViewItem* item = [[ReadingListCollectionViewItem alloc]
           initWithType:0
                    url:url
      distillationState:reading_list::UIStatusFromModelStatus(
                            entry->DistilledState())];

  item.faviconPageURL =
      entry->DistilledURL().is_valid() ? entry->DistilledURL() : url;

  BOOL has_distillation_details =
      entry->DistilledState() == ReadingListEntry::PROCESSED &&
      entry->DistillationSize() != 0 && entry->DistillationTime() != 0;
  NSString* title = base::SysUTF8ToNSString(entry->Title());
  if ([title length]) {
    item.title = title;
  } else {
    item.title =
        base::SysUTF16ToNSString(url_formatter::FormatUrl(url.GetOrigin()));
  }
  item.subtitle =
      base::SysUTF16ToNSString(url_formatter::FormatUrl(url.GetOrigin()));
  item.distillationDate =
      has_distillation_details ? entry->DistillationTime() : 0;
  item.distillationSize =
      has_distillation_details ? entry->DistillationSize() : 0;
  item.accessibilityDelegate = delegate;
  return item;
}

// Whether the data source has changed.
- (BOOL)hasDataSourceChanged {
  NSMutableArray<CollectionViewItem*>* readArray = [NSMutableArray array];
  NSMutableArray<CollectionViewItem*>* unreadArray = [NSMutableArray array];
  [self fillReadItems:readArray unreadItems:unreadArray withDelegate:nil];

  return [self currentSection:[self.dataSink readItems]
             isDifferentOfArray:readArray] ||
         [self currentSection:[self.dataSink unreadItems]
             isDifferentOfArray:unreadArray];
}

// Returns whether there is a difference between the elements contained in the
// |sectionIdentifier| and those in the |array|. The comparison is done with the
// URL of the elements. If an element exist in both, the one in |currentSection|
// will be overwriten with the informations contained in the one from|array|.
- (BOOL)currentSection:(NSArray<CollectionViewItem*>*)currentSection
    isDifferentOfArray:(NSArray<CollectionViewItem*>*)array {
  if (currentSection.count != array.count)
    return YES;

  NSMutableArray<ReadingListCollectionViewItem*>* itemsToReconfigure =
      [NSMutableArray array];

  NSInteger index = 0;
  for (ReadingListCollectionViewItem* newItem in array) {
    ReadingListCollectionViewItem* oldItem =
        base::mac::ObjCCastStrict<ReadingListCollectionViewItem>(
            currentSection[index]);
    if (oldItem.url == newItem.url) {
      if (![oldItem isEqual:newItem]) {
        oldItem.title = newItem.title;
        oldItem.subtitle = newItem.subtitle;
        oldItem.distillationState = newItem.distillationState;
        oldItem.distillationDate = newItem.distillationDate;
        oldItem.distillationSize = newItem.distillationSize;
        [itemsToReconfigure addObject:oldItem];
      }
      if (oldItem.faviconPageURL != newItem.faviconPageURL) {
        oldItem.faviconPageURL = newItem.faviconPageURL;
        [self fetchFaviconForItem:oldItem];
      }
    }
    if (![oldItem isEqual:newItem]) {
      return YES;
    }
    index++;
  }
  [self.dataSink itemsHaveChanged:itemsToReconfigure];
  return NO;
}

// Logs the deletions histograms for the entry associated with |item|.
- (void)logDeletionOfItem:(CollectionViewItem*)item {
  const ReadingListEntry* entry = [self entryFromItem:item];

  if (!entry)
    return;

  int64_t firstRead = entry->FirstReadTime();
  if (firstRead > 0) {
    // Log 0 if the entry has never been read.
    firstRead = (base::Time::Now() - base::Time::UnixEpoch()).InMicroseconds() -
                firstRead;
    // Convert it to hours.
    firstRead = firstRead / base::Time::kMicrosecondsPerHour;
  }
  UMA_HISTOGRAM_COUNTS_10000("ReadingList.FirstReadAgeOnDeletion", firstRead);

  int64_t age = (base::Time::Now() - base::Time::UnixEpoch()).InMicroseconds() -
                entry->CreationTime();
  // Convert it to hours.
  age = age / base::Time::kMicrosecondsPerHour;
  if (entry->IsRead())
    UMA_HISTOGRAM_COUNTS_10000("ReadingList.Read.AgeOnDeletion", age);
  else
    UMA_HISTOGRAM_COUNTS_10000("ReadingList.Unread.AgeOnDeletion", age);
}

@end
