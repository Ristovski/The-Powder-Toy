#include "GetUserInfoRequest.h"

namespace http
{
	GetUserInfoRequest::GetUserInfoRequest(ByteString username) :
		APIRequest("http://" SERVER "/User.json?Name=" + username)
	{
	}

	GetUserInfoRequest::~GetUserInfoRequest()
	{
	}

	std::unique_ptr<UserInfo> GetUserInfoRequest::Finish()
	{
		std::unique_ptr<UserInfo> user_info;
		auto result = APIRequest::Finish();
		if (result.document)
		{
			auto &user = (*result.document)["User"];
			user_info = std::unique_ptr<UserInfo>(new UserInfo(
				user["ID"].asInt(),
				user["Age"].asInt(),
				user["Username"].asString(),
				ByteString(user["Biography"].asString()).FromUtf8(),
				ByteString(user["Location"].asString()).FromUtf8(),
				user["Website"].asString(),
				user["Saves"]["Count"].asInt(),
				user["Saves"]["AverageScore"].asInt(),
				user["Saves"]["HighestScore"].asInt(),
				user["Forum"]["Topics"].asInt(),
				user["Forum"]["Replies"].asInt(),
				user["Forum"]["Reputation"].asInt()
			));
		}
		return user_info;
	}
}
