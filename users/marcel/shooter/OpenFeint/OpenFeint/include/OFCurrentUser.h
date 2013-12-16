//  Copyright 2009-2010 Aurora Feint, Inc.
// 
//  Licensed under the Apache License, Version 2.0 (the "License");
//  you may not use this file except in compliance with the License.
//  You may obtain a copy of the License at
//  
//  	http://www.apache.org/licenses/LICENSE-2.0
//  	
//  Unless required by applicable law or agreed to in writing, software
//  distributed under the License is distributed on an "AS IS" BASIS,
//  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
//  See the License for the specific language governing permissions and
//  limitations under the License.

@class OFUser;
@class OFRequestHandle;
#import "OFCallbackable.h"

@protocol OFCurrentUserDelegate;

//////////////////////////////////////////////////////////////////////////////////////////
/// The public interface for OFCurrentUser lets to query and manipulate certain properties
/// of the current user.
//////////////////////////////////////////////////////////////////////////////////////////
@interface OFCurrentUser : NSObject<OFCallbackable>

//////////////////////////////////////////////////////////////////////////////////////////
/// Set a delegate for all OFCurrentUser related actions. Must adopt the 
/// OFCurrentUserDelegate protocol.
///
/// @note Defaults to nil. Weak reference
//////////////////////////////////////////////////////////////////////////////////////////
+ (void)setDelegate:(id<OFCurrentUserDelegate>)delegate;

//////////////////////////////////////////////////////////////////////////////////////////
/// Get the current user.
///
/// @return OFUser*		The current User Object.
//////////////////////////////////////////////////////////////////////////////////////////
+ (OFUser*)currentUser;

//////////////////////////////////////////////////////////////////////////////////////////
/// Make this game a favorite of the current user.  The review text is optional.
///
/// @param reviewText		Review text for the this game that the user just favorited.
///							This is optional.  Pass nil to ignore.
///
/// @return OFRequestHandle for the server request.  Use this to cancel the request
///
/// @note Invokes			- (void)didFavoriteCurrentGame on success and
///							- (void)didFailFavoriteCurrentGame on failure.
//////////////////////////////////////////////////////////////////////////////////////////
+ (OFRequestHandle*)favoriteCurrentGame:(NSString*)reviewText;

//////////////////////////////////////////////////////////////////////////////////////////
/// Make the current user start following another user
///
/// @param user		user to follow
///
/// @return OFRequestHandle for the server request.  Use this to cancel the request
///
/// @note Inovkes	- (void)didSendFriendRequest on success and
///					- (void)didFailSendFriendRequest on failure.
///					These function are part of the OFUserDelegate protocol
//////////////////////////////////////////////////////////////////////////////////////////
+ (OFRequestHandle*)sendFriendRequest:(OFUser*)user;

//////////////////////////////////////////////////////////////////////////////////////////
/// Make the current user stop following another user
///
/// @param user		user to stop following
///
/// @return OFRequestHandle for the server request.  Use this to cancel the request
///
/// @note Invokes	- (void)didUnfriend on success and
///					- (void)didFailUnfriend on failure
//////////////////////////////////////////////////////////////////////////////////////////
+ (OFRequestHandle*)unfriend:(OFUser*)user;

//////////////////////////////////////////////////////////////////////////////////////////
/// Tells whether the current player has connected their OpenFeint Account to any social network
///
/// @return OFRequestHandle for the server request.  User this to cancel the request
///
/// @note Invokes
//////////////////////////////////////////////////////////////////////////////////////////
+ (OFRequestHandle*)checkConnectedToSocialNetwork;

//////////////////////////////////////////////////////////////////////////////////////////
/// Whether or not the current user has OpenFeint friends
///
/// @return BOOL		@c YES if the current user has OpenFeint friends
//////////////////////////////////////////////////////////////////////////////////////////
+ (BOOL)hasFriends;

///////////////////////////////////////////////////////////////////////////
/// The number of unviewed Challenges by the current user.
///
/// @return uint		The number of unview Challenges by the current user.
///////////////////////////////////////////////////////////////////////////
+ (NSInteger)unviewedChallengesCount;

///////////////////////////////////////////////////////////////////////////
/// Whether or not the current user allows social notifications  to be posted automatically.
///
/// @return BOOL		@c YES if the current user allows social notifications
///////////////////////////////////////////////////////////////////////////
+ (BOOL)allowsAutoSocialNotifications;

///////////////////////////////////////////////////////////////////////////
/// How many things are waiting for this user in OpenFeint Dashboard.
///
/// @return uint		returns pending friends + unview challenges + new inbox items.
///////////////////////////////////////////////////////////////////////////
+ (NSInteger)OpenFeintBadgeCount;

@end

//////////////////////////////////////////////////////////////////////////////////////////
/// Adopt the OFCurrentUserDelegate Protocol to receive information regarding OFCurrentUser.  You must
/// call OFCurrentUser's +(void)setDelegate: method to receive information
//////////////////////////////////////////////////////////////////////////////////////////
@protocol OFCurrentUserDelegate
@optional

//////////////////////////////////////////////////////////////////////////////////////////
/// Invoked when favoriteCurrentGame: successfully completes
//////////////////////////////////////////////////////////////////////////////////////////
- (void)didFavoriteCurrentGame;

//////////////////////////////////////////////////////////////////////////////////////////
/// Invoked when favoriteCurrentGame: fails.
//////////////////////////////////////////////////////////////////////////////////////////
- (void)didFailFavoriteCurrentGame;

//////////////////////////////////////////////////////////////////////////////////////////
/// Invoked when sendFriendRequest: successfully completes.
//////////////////////////////////////////////////////////////////////////////////////////
- (void)didSendFriendRequest;

//////////////////////////////////////////////////////////////////////////////////////////
/// Invoked when sendFriendRequest: fails.
//////////////////////////////////////////////////////////////////////////////////////////
- (void)didFailSendFriendRequest;

//////////////////////////////////////////////////////////////////////////////////////////
/// Invoked when unfriend: successfully completes.
//////////////////////////////////////////////////////////////////////////////////////////
- (void)didUnfriend;

//////////////////////////////////////////////////////////////////////////////////////////
/// Invoked when unfriend: fails.
//////////////////////////////////////////////////////////////////////////////////////////
- (void)didFailUnfriend;

//////////////////////////////////////////////////////////////////////////////////////////
/// Invoked when checkConnectedToSocialNetwork successfully completes
///
/// @param connected		@c YES if the current user's OpenFeint account is connected to any social networks
//////////////////////////////////////////////////////////////////////////////////////////
- (void)didCheckConnectedToSocialNetwork:(BOOL)connected;

//////////////////////////////////////////////////////////////////////////////////////////
/// Invoked when checkConnectedToSocialNetwork fails;
//////////////////////////////////////////////////////////////////////////////////////////
- (void)didFailCheckConnectedToSocialNetwork;

@end




