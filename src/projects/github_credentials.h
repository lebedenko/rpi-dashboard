#pragma once

#include <QByteArray>
#include <QString>

namespace dashboard::projects {

struct GitHubCredential {
  QByteArray token;
  QString diagnostic;
};

[[nodiscard]] GitHubCredential loadGitHubCredential(const QByteArray& token_file_path,
                                                    const QByteArray& environment_token);

}  // namespace dashboard::projects
