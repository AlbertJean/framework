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


#import "OFCallbackable.h"
#import "OFResource.h"
#import "OFSqlQuery.h"
#import "OFPointer.h"

@class OFRequestHandle;
class OFHttpService;
class OFImageViewHttpServiceObserver;

@protocol OFUserDelegate;

//////////////////////////////////////////////////////////////////////////////////////////
/// The public interface for OFUser let you query friends, manipulate the friends
/// of the current user, and query options on the current user.  Note that Friends are
/// people which you have sent a friend invite to or accepted a friend invite from.  This
/// means as soon as you friend request someone they are immediately your friend, the other
/// party does not need to accept the friend request before they are your friend.
//////////////////////////////////////////////////////////////////////////////////////////
@interface OFUser : OFResource<NSCoding, OFCallbackable>
{
	@package
	NSString* name;
	NSString* profilePictureUrl;
	NSString* profilePictureSource;
	BOOL usesFacebookProfilePicture;
	NSString* lastPlayedGameId;
	NSString* lastPlayedGameName;
	BOOL followsLocalUser;
	BOOL followedByLocalUser;
	NSUInteger gamerScore;
	BOOL online;
	double latitude;
	double longitude;
	
	OFPointer<OFHttpService> mHttpService;
	OFPointer<OFImageViewHttpServiceObserver> mHttpServiceObserver;
}

//////////////////////////////////////////////////////////////////////////////////////////
/// Set a delegate for all OFUser related actions. Must adopt the 
/// OFUserDelegate protocol.
///
/// @note Defaults to nil. Weak reference
//////////////////////////////////////////////////////////////////////////////////////////
+ (void)setDelegate:(id<OFUserDelegate>)delegate;

//////////////////////////////////////////////////////////////////////////////////////////
/// Get a user by their userId
///
/// @param userId		The user Id of the user you wish to get
///
/// @return OFRequestHandle for the server request.  Use this to cancel the request
///
/// @note Invokes		- (void)didGetUser:(OFUser*)user; on success and
///						- (void)didFailGetUser; on failure
//////////////////////////////////////////////////////////////////////////////////////////
+ (OFRequestHandle*)getUser:(NSString*)userId;

//////////////////////////////////////////////////////////////////////////////////////////
/// Get the users which this user follows
///
/// @return OFRequestHandle for the server request.  Use this to cancel the request
///
/// @note Invokes	- (void)didGetFriends:(NSArray*)follows OFUser:(OFUser*)user on success and
///					- (void)didFailGetFriendsOFUser:(OFUser*)user on failure
//////////////////////////////////////////////////////////////////////////////////////////
- (OFRequestHandle*)getFriends;

//////////////////////////////////////////////////////////////////////////////////////////
/// Get the users which this user follows who also have this application.
///
/// @return OFRequestHandle for the server request.  Use this to cancel the request
///
/// @note Invokes	- (void)didGetFriendsWithThisApplication:(NSArray*)follows OFUser:(OFUser*)user on success and
///					- (void)didFailGetFriendsWithThisApplicationOFUser:(OFUser*)user on failure
//////////////////////////////////////////////////////////////////////////////////////////
- (OFRequestHandle*)getFriendsWithThisApplication;

//////////////////////////////////////////////////////////////////////////////////////////
/// Get the profile picture for the OFUser
///
/// @return OFRequestHandle for the server request.  Use this to cancel the request
///
/// @note Invokes	- (void)didGetProfilePicture:(UIImage*)image OFUser:(OFUser*)user; on success and
///					- (void)didFailGetProfilePictureOFUser:(OFUser*)user; on failure.
//////////////////////////////////////////////////////////////////////////////////////////
- (OFRequestHandle*)getProfilePicture;

//////////////////////////////////////////////////////////////////////////////////////////
/// Name of this User
//////////////////////////////////////////////////////////////////////////////////////////
@property (nonatomic, readonly) NSString* name;

//////////////////////////////////////////////////////////////////////////////////////////
/// Application Id of the last game played
//////////////////////////////////////////////////////////////////////////////////////////
@property (nonatomic, readonly)	NSString* lastPlayedGameId;

//////////////////////////////////////////////////////////////////////////////////////////
/// Name of the last game played.
//////////////////////////////////////////////////////////////////////////////////////////
@property (nonatomic, readonly) NSString* lastPlayedGameName;

//////////////////////////////////////////////////////////////////////////////////////////
/// Gamer Score of this user
//////////////////////////////////////////////////////////////////////////////////////////
@property (nonatomic, readonly)	NSUInteger gamerScore;

//////////////////////////////////////////////////////////////////////////////////////////
/// @c YES if this user follows the current user
//////////////////////////////////////////////////////////////////////////////////////////
@property (nonatomic, readonly)	BOOL followsLocalUser;

//////////////////////////////////////////////////////////////////////////////////////////
/// @c YES if the current user follows this user
//////////////////////////////////////////////////////////////////////////////////////////
@property (nonatomic, readonly)	BOOL followedByLocalUser;

//////////////////////////////////////////////////////////////////////////////////////////
/// @c YES if this user is currently online.
//////////////////////////////////////////////////////////////////////////////////////////
@property (nonatomic, readonly)	BOOL online;

//////////////////////////////////////////////////////////////////////////////////////////
/// User id associated with this user.
//////////////////////////////////////////////////////////////////////////////////////////
@property (nonatomic, readonly) NSString* userId;

//////////////////////////////////////////////////////////////////////////////////////////
/// @internal
//////////////////////////////////////////////////////////////////////////////////////////
@property (nonatomic, readonly, retain) NSString* profilePictureUrl;
@property (nonatomic, readonly, retain) NSString* profilePictureSource;
@property (nonatomic, readonly) BOOL usesFacebookProfilePicture;
@property (nonatomic, readonly)	double latitude;
@property (nonatomic, readonly)	double longitude;

- (id)initWithLocalSQL:(OFSqlQuery*)queryRow;
- (id)initWithCoder:(NSCoder *)aDecoder;
- (void)encodeWithCoder:(NSCoder *)aCoder;
+ (id)invalidUser;
+ (NSString*)getResourceName;
- (bool)isLocalUser;
- (void)adjustGamerscore:(int)gamerscoreAdjustment;
- (void)changeProfilePictureUrl:(NSString*)url facebook:(BOOL)isFacebook twitter:(BOOL)isTwitter uploaded:(BOOL)isUploaded;
- (void)setName:(NSString*)value;
- (void)setFollowedByLocalUser:(BOOL)value;

@end


//////////////////////////////////////////////////////////////////////////////////////////
/// Adopt the OFUserDelegate Protocol to receive information regarding OFUser.  You must
/// call OFUser's +(void)setDelegate: method to receive information
//////////////////////////////////////////////////////////////////////////////////////////
@protocol OFUserDelegate
@optional

//////////////////////////////////////////////////////////////////////////////////////////
/// Invoked when getUser: successfully completes
///
/// @param user		The user requested
//////////////////////////////////////////////////////////////////////////////////////////
- (void)didGetUser:(OFUser*)user;

//////////////////////////////////////////////////////////////////////////////////////////
/// Invoked when getUser: fails.
//////////////////////////////////////////////////////////////////////////////////////////
- (void)didFailGetUser;

//////////////////////////////////////////////////////////////////////////////////////////
/// Invoked when getFriends successfully completes
///
/// @param follows		The NSArray of OFUsers which are friends
/// @param user			The OFUser for which the friends were requested
//////////////////////////////////////////////////////////////////////////////////////////
- (void)didGetFriends:(NSArray*)follows OFUser:(OFUser*)user;

//////////////////////////////////////////////////////////////////////////////////////////
/// Invoked when getFriends fails.
///
/// @param user		The OFUser for which we failted to get friends
//////////////////////////////////////////////////////////////////////////////////////////
- (void)didFailGetFriendsOFUser:(OFUser*)user;

//////////////////////////////////////////////////////////////////////////////////////////
/// Invoked when getFriendsWithThisApplication successfully complete.
///
/// @param follow		The NSArray of OFUsers which are friends
/// @param user			The OFUser for which the friends were requested.
//////////////////////////////////////////////////////////////////////////////////////////
- (void)didGetFriendsWithThisApplication:(NSArray*)follows OFUser:(OFUser*)user;

//////////////////////////////////////////////////////////////////////////////////////////
/// Invoked when getFriendsWithThisApplication fails.
///
/// @param user			The OFUser for which we failed to get friends
//////////////////////////////////////////////////////////////////////////////////////////
- (void)didFailGetFriendsWithThisApplicationOFUser:(OFUser*)user;

//////////////////////////////////////////////////////////////////////////////////////////
/// Invoked when getProfilePicture successfully completes
///
/// @param image		The image requested
/// @param user			The OFUser that this image belongs to
//////////////////////////////////////////////////////////////////////////////////////////
- (void)didGetProfilePicture:(UIImage*)image OFUser:(OFUser*)user;

//////////////////////////////////////////////////////////////////////////////////////////
/// Invoked when getProfilePicture fails
///
/// @param user			The OFUser for which the image was requested.
//////////////////////////////////////////////////////////////////////////////////////////
- (void)didFailGetProfilePictureOFUser:(OFUser*)user;


@end



