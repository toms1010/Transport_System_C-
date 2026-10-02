#pragma once

#include <string>

namespace st {

class Notice {
public:
    Notice() = default;

    Notice(std::string noticeId, std::string title, std::string content, std::string authorId,
           std::string createdAt)
        : noticeId_(std::move(noticeId)),
          title_(std::move(title)),
          content_(std::move(content)),
          authorId_(std::move(authorId)),
          createdAt_(std::move(createdAt)) {}

    const std::string& noticeId() const { return noticeId_; }
    const std::string& title() const { return title_; }
    const std::string& content() const { return content_; }
    const std::string& authorId() const { return authorId_; }
    const std::string& createdAt() const { return createdAt_; }

    void setNoticeId(std::string v) { noticeId_ = std::move(v); }
    void setTitle(std::string v) { title_ = std::move(v); }
    void setContent(std::string v) { content_ = std::move(v); }
    void setAuthorId(std::string v) { authorId_ = std::move(v); }
    void setCreatedAt(std::string v) { createdAt_ = std::move(v); }

private:
    std::string noticeId_;
    std::string title_;
    std::string content_;
    std::string authorId_;
    std::string createdAt_;
};

}  // namespace st