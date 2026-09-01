#pragma once

#include <QNetworkReply>

#include <algorithm>
#include <memory>

namespace dashboard::network {

class BoundedReply final {
 public:
  static std::shared_ptr<BoundedReply> collect(QNetworkReply& reply, qsizetype maximum_bytes) {
    auto buffer = std::shared_ptr<BoundedReply>(new BoundedReply(reply, maximum_bytes));
    QObject::connect(&reply, &QIODevice::readyRead, &reply, [buffer] { buffer->consume(); });
    QObject::connect(&reply, &QNetworkReply::metaDataChanged, &reply, [buffer] {
      if (buffer->reply_->header(QNetworkRequest::ContentLengthHeader).toLongLong() > buffer->maximum_bytes_) {
        buffer->reject();
      }
    });
    return buffer;
  }

  void finish() { consume(); }
  [[nodiscard]] bool exceededLimit() const { return exceeded_limit_; }
  [[nodiscard]] const QByteArray& body() const { return body_; }

 private:
  BoundedReply(QNetworkReply& reply, qsizetype maximum_bytes) : reply_(&reply), maximum_bytes_(maximum_bytes) {
    reply_->setReadBufferSize(maximum_bytes + 1);
  }
  void consume() {
    while (!exceeded_limit_ && reply_->bytesAvailable() > 0) {
      const qsizetype remaining = maximum_bytes_ - body_.size();
      body_.append(reply_->read((std::min)(reply_->bytesAvailable(), remaining + 1)));
      if (body_.size() > maximum_bytes_) {
        reject();
      }
    }
  }
  void reject() {
    exceeded_limit_ = true;
    body_.clear();
    reply_->abort();
  }

  QNetworkReply* reply_;
  qsizetype maximum_bytes_;
  QByteArray body_;
  bool exceeded_limit_{};
};

}  // namespace dashboard::network
