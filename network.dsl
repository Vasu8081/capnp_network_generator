message NetworkMessage(1) {
    MessageType msgType;
}

message Request(2) extends NetworkMessage {
    int requestId;
    string userId;
    string authToken;
}

message Response(3) extends NetworkMessage {
    int requestId;
    Status statusCode;
    int errorCode;
    string errorMessage;
}

message YoutubeVideo(4) extends NetworkMessage {
    string dbId;
    string videoId;
    string title;
    string thumbnail;
    string thumbnailMedium;
    string thumbnailHigh;
    string thumbnailStandard;
    string thumbnailMaxres;
}

message YoutubeVideoSnapshotRequest(5) extends Request {
    list<string> videoIds;
}

message YoutubeVideoSnapshotResponse(6) extends Response {
    list<YoutubeVideo> videos;
}

message YoutubeVideoHeartbeat(7) extends NetworkMessage {
    int videosCount;
}

message YoutubeVideoUpdates(8) extends NetworkMessage {
    list<YoutubeVideo> videos;
}

message AddYoutubeVideosRequest(9) extends Request {
    list<YoutubeVideo> videos;
}

message AddYoutubeVideosResponse(10) extends Response {
    list<YoutubeVideo> videos;
}

message ModifyYoutubeVideosRequest(11) extends Request {
    list<YoutubeVideo> videos;
}

message ModifyYoutubeVideosResponse(12) extends Response {
    list<YoutubeVideo> videos;
}

message DeleteYoutubeVideosRequest(13) extends Request {
    list<string> deleteVideoIds;
}

message DeleteYoutubeVideosResponse(14) extends Response {
    list<string> deletedVideoIds;
}

message Thumbnail(15) extends NetworkMessage {
    string imageId;
    string mime;
    int width;
    int height;
    int size;
    Data payload;
    Data sha256;
}

message Blog(16) extends NetworkMessage {
    string dbId;
    string title;
    string description;
    string contentType;
    string content;
    Thumbnail thumbnail;
}

message BlogSnapshotRequest(17) extends Request {
    list<string> ids;
}

message BlogSnapshotResponse(18) extends Response {
    list<Blog> blogs;
}

message BlogHeartbeat(19) extends NetworkMessage {
    int blogsCount;
}

message BlogUpdates(20) extends NetworkMessage {
    list<Blog> blogs;
}

message AddBlogsRequest(21) extends Request {
    list<Blog> blogs;
}

message AddBlogsResponse(22) extends Response {
    list<Blog> blogs;
}

message ModifyBlogsRequest(23) extends Request {
    list<Blog> blogs;
}

message ModifyBlogsResponse(24) extends Response {
    list<Blog> blogs;
}

message DeleteBlogsRequest(25) extends Request {
    list<string> deleteBlogIds;
}

message DeleteBlogsResponse(26) extends Response {
    list<string> deletedBlogIds;
}

message Goal(27) extends NetworkMessage {
    string dbId;
    string goalName;
    string deadline;
    string label;
    string handleUrl;
    int goal;
    map<string, string> dailyCounts;
}

message GoalsSnapshotRequest(28) extends Request {
    list<string> goalNames;
}

message GoalsSnapshotResponse(29) extends Response {
    list<Goal> goals;
}

message AddGoalsRequest(30) extends Request {
    list<Goal> goals;
}

message AddGoalsResponse(31) extends Response {
    list<Goal> goals;
}

message ModifyGoalsRequest(32) extends Request {
    list<Goal> goals;
}

message ModifyGoalsResponse(33) extends Response {
    list<Goal> goals;
}

message DeleteGoalsRequest(34) extends Request {
    list<string> deletegoalIds;
}

message DeleteGoalsResponse(35) extends Response {
    list<string> deletedgoalIds;
}

message LoginRequest(36) extends Request {
    string username;
    string passwordEncoded;
}

message LoginResponse(37) extends Response {
    string authToken;
}

message Entitlement(38) extends NetworkMessage {
    string dbId;
    string key;
    string description;
}

message Group(39) extends NetworkMessage {
    string dbId;
    string name;
    string description;
    list<string> usernames;
    list<Entitlement> entitlements;
}

message User(40) extends NetworkMessage {
    string dbId;
    string username;
    string email;
    string passwordEncoded;
    bool emailVerified;
    list<Group> groups;
    list<Entitlement> entitlements;
}

message RegisterUserRequest(41) extends Request {
    User user;
}

message CreateGroupRequest(42) extends Request {
    Group group;
}

message CreateEntitlementRequest(43) extends Request {
    Entitlement entitlement;
}

message ModifyUserRequest(44) extends Request {
    User user;
}

message ModifyGroupRequest(45) extends Request {
    Group group;
}

message ModifyEntitlementRequest(46) extends Request {
    Entitlement entitlements;
}

message DeleteUserRequest(47) extends Request {
    string userName;
}

message DeleteGroupRequest(48) extends Request {
    string groupName;
}

message DeleteEntitlementRequest(49) extends Request {
    string entitlementKey;
}

message SecurityDashboardSnapshotRequest(50) extends Request {
    bool usersSnapshot;
    bool groupsSnapshot;
    bool entitlementSnapshot;
}

message SecurityDashboardSnapshotResponse(51) extends Response {
    list<User> users;
    list<Group> groups;
    list<Entitlement> entitlements;
}