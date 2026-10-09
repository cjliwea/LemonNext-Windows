/*
 * SPDX-FileCopyrightText: 2026 Project LemonLime Online
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "probleminstaller.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QObject>

namespace {

auto copyOneFile(const QString &source, const QString &target, ProblemInstaller::Overwrite policy,
                 ProblemInstaller::Report *report, QString *error) -> bool {
	const QFileInfo targetInfo(target);

	if (! QDir().mkpath(targetInfo.absolutePath())) {
		if (error)
			*error = QObject::tr("无法创建目录 %1").arg(QDir::toNativeSeparators(targetInfo.absolutePath()));

		++report->failed;
		return false;
	}

	if (targetInfo.exists()) {
		if (policy == ProblemInstaller::Skip) {
			++report->skipped;
			return true;
		}

		if (! QFile::remove(target)) {
			if (error)
				*error = QObject::tr("无法覆盖已存在的文件 %1").arg(QDir::toNativeSeparators(target));

			++report->failed;
			return false;
		}
	}

	if (! QFile::copy(source, target)) {
		if (error)
			*error = QObject::tr("复制 %1 失败").arg(QDir::toNativeSeparators(source));

		++report->failed;
		return false;
	}

	++report->written;
	return true;
}

} // namespace

ProblemInstaller::ProblemInstaller() = default;

void ProblemInstaller::setContestDir(const QString &dir) { contestDir_ = dir; }

void ProblemInstaller::setOverwrite(Overwrite policy) { policy_ = policy; }

auto ProblemInstaller::targetDir(const QString &englishName) const -> QString {
	const QString dataDir = QDir(contestDir_).absoluteFilePath(QStringLiteral("data"));

	return QDir(dataDir).absoluteFilePath(englishName);
}

auto ProblemInstaller::install(const ScannedProblem &problem, Report *report, QString *error) const -> bool {
	if (contestDir_.isEmpty()) {
		if (error)
			*error = QObject::tr("没有指定比赛目录");

		return false;
	}

	if (! ContestScanner::isSafePathComponent(problem.englishName)) {
		if (error)
			*error = QObject::tr("英文名 / 源文件名「%1」不是合法的文件夹名").arg(problem.englishName);

		return false;
	}

	if (problem.cases.isEmpty() && ! problem.choiceProblem) {
		if (error)
			*error = QObject::tr("题目「%1」没有任何测试点").arg(problem.title);

		return false;
	}

	const QString destination = targetDir(problem.englishName);

	if (! QDir().mkpath(destination)) {
		if (error)
			*error = QObject::tr("无法创建题目目录 %1").arg(QDir::toNativeSeparators(destination));

		return false;
	}

	// 选择题没有测试点：把 paper.md / key.json 以及老师放在同一层的题面素材
	// （插图等）原样搬进 data/<源文件名>/
	if (problem.choiceProblem) {
		const QFileInfoList entries = QDir(problem.dirPath).entryInfoList(QDir::Files | QDir::NoSymLinks);

		for (const QFileInfo &one : entries) {
			copyOneFile(one.absoluteFilePath(), QDir(destination).absoluteFilePath(one.fileName()),
			            policy_, report, error);
		}

		if (report->failed > 0) {
			report->messages.append(QObject::tr("选择题「%1」有 %2 个文件复制失败。")
			                            .arg(problem.englishName)
			                            .arg(report->failed));
			return false;
		}

		return true;
	}

	const QDir sourceRoot(problem.dirPath);

	for (const ScannedCase &one : problem.cases) {
		const QString sourceInput = sourceRoot.absoluteFilePath(one.inputRel);
		const QString sourceOutput = sourceRoot.absoluteFilePath(one.outputRel);
		const QString targetInput = QDir(destination).absoluteFilePath(one.inputRel);
		const QString targetOutput = QDir(destination).absoluteFilePath(one.outputRel);

		copyOneFile(sourceInput, targetInput, policy_, report, error);
		copyOneFile(sourceOutput, targetOutput, policy_, report, error);
		++report->copiedCases;
	}

	if (report->failed > 0) {
		report->messages.append(QObject::tr("题目「%1」有 %2 个文件复制失败。")
		                            .arg(problem.englishName)
		                            .arg(report->failed));
		return false;
	}

	return true;
}
