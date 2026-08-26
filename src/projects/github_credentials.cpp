#include "projects/github_credentials.h"

#include <QFile>

namespace dashboard::projects {

GitHubCredential loadGitHubCredential(const QByteArray& token_file_path, const QByteArray& environment_token) {
  if (token_file_path.isEmpty()) {
    return {.token = environment_token, .diagnostic = {}};
  }

  QFile credential_file(QFile::decodeName(token_file_path));
  if (!credential_file.open(QIODevice::ReadOnly)) {
    return {.token = {},
            .diagnostic = QStringLiteral("GitHub credential file could not be read; using anonymous access")};
  }

  auto token = credential_file.readAll();
  while (token.endsWith('\n') || token.endsWith('\r')) {
    token.chop(1);
  }
  if (token.isEmpty()) {
    return {.token = {}, .diagnostic = QStringLiteral("GitHub credential file is empty; using anonymous access")};
  }
  return {.token = std::move(token), .diagnostic = {}};
}

}  // namespace dashboard::projects
