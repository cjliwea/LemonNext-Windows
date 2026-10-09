/*
 * SPDX-FileCopyrightText: 2026 Project LemonLime Online
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "choicejudge.h"

#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonParseError>
#include <QJsonValue>
#include <QObject>
#include <QRegularExpression>
#include <QSet>

#include <cmath>

namespace {

// 选择题判分里所有数值都可能是半分制下的 1.5，所以内部一律用 double，
// 输出时才决定怎么显示。
auto numberToText(double value) -> QString {
	const double rounded = std::round(value);

	if (std::fabs(value - rounded) < 1e-9)
		return QString::number(static_cast<qint64>(rounded));

	return QString::number(value, 'g', 6);
}

// 选择题答案只保留字母与数字，并统一大写：'a b' / 'A,B' / 'AB' 都算同一个答案
auto lettersOf(const QString &raw) -> QString {
	QString out;
	out.reserve(raw.size());

	for (const QChar ch : raw) {
		if (ch.isLetterOrNumber())
			out.append(ch.toUpper());
	}

	return out;
}

// 把 JSON 里的 answer 字段拉平成字符串列表：可能是 "A"，也可能是 ["A","B"]
auto valueToStringList(const QJsonValue &value) -> QStringList {
	QStringList out;

	if (value.isArray()) {
		const QJsonArray array = value.toArray();

		for (const QJsonValue &one : array) {
			if (one.isString())
				out.append(one.toString());
			else if (one.isDouble())
				out.append(numberToText(one.toDouble()));
			else if (one.isBool())
				out.append(one.toBool() ? QStringLiteral("T") : QStringLiteral("F"));
		}
	} else if (value.isString()) {
		out.append(value.toString());
	} else if (value.isDouble()) {
		out.append(numberToText(value.toDouble()));
	} else if (value.isBool()) {
		out.append(value.toBool() ? QStringLiteral("T") : QStringLiteral("F"));
	}

	return out;
}

// 填空题的一行答案里可能有多种写法，用 ; , ， ； | / 分隔
auto splitFillVariants(const QStringList &raw) -> QStringList {
	static const QRegularExpression separator(QStringLiteral("[;；,，|/]"));

	QStringList out;

	for (const QString &one : raw) {
		const QStringList parts = one.split(separator, Qt::SkipEmptyParts);

		if (parts.isEmpty()) {
			out.append(one);
			continue;
		}

		for (const QString &part : parts)
			out.append(part);
	}

	return out;
}

} // namespace

auto ChoiceJudge::parseTypeName(const QString &raw) -> QuestionType {
	const QString key = raw.trimmed().toLower();

	if (key == QStringLiteral("multiple") || key == QStringLiteral("multi"))
		return QuestionType::Multiple;

	if (key == QStringLiteral("truefalse") || key == QStringLiteral("tf") ||
	    key == QStringLiteral("judge"))
		return QuestionType::TrueFalse;

	if (key == QStringLiteral("fill"))
		return QuestionType::Fill;

	return QuestionType::Single;
}

auto ChoiceJudge::typeName(QuestionType type) -> QString {
	switch (type) {
		case QuestionType::Multiple:
			return QObject::tr("多选");
		case QuestionType::TrueFalse:
			return QObject::tr("判断");
		case QuestionType::Fill:
			return QObject::tr("填空");
		case QuestionType::Single:
			break;
	}

	return QObject::tr("单选");
}

auto ChoiceJudge::normalizeAnswer(QuestionType type, const QStringList &raw, bool caseSensitive)
    -> QStringList {
	QStringList out;

	if (type == QuestionType::Fill) {
		for (const QString &one : splitFillVariants(raw)) {
			QString value = one.trimmed();

			if (! caseSensitive)
				value = value.toCaseFolded();

			if (! value.isEmpty())
				out.append(value);
		}

		return out;
	}

	// 单选 / 多选 / 判断：只保留字母数字并大写
	for (const QString &one : raw) {
		const QString letters = lettersOf(one);

		if (! letters.isEmpty())
			out.append(letters);
	}

	return out;
}

auto ChoiceJudge::parseKey(const QJsonObject &root, QString *errorOut) -> QList<Question> {
	QList<Question> questions;

	if (! root.contains(QStringLiteral("questions")) ||
	    ! root.value(QStringLiteral("questions")).isArray()) {
		if (errorOut)
			*errorOut = QObject::tr("key.json 没有题目");

		return questions;
	}

	const double defaultScore = root.value(QStringLiteral("default_score")).toDouble(0.0);
	const QJsonArray array = root.value(QStringLiteral("questions")).toArray();
	int index = 0;

	for (const QJsonValue &one : array) {
		++index;

		if (! one.isObject()) {
			if (errorOut)
				*errorOut = QObject::tr("第 %1 项不是合法的题目对象").arg(index);

			return {};
		}

		const QJsonObject obj = one.toObject();
		Question question;
		question.id = obj.value(QStringLiteral("id")).toInt(index);
		question.type = parseTypeName(obj.value(QStringLiteral("type")).toString());
		question.score = obj.value(QStringLiteral("score")).toDouble(defaultScore);
		question.partial =
		    obj.value(QStringLiteral("scoring")).toString().trimmed().toLower() ==
		    QStringLiteral("partial");
		question.caseSensitive = obj.value(QStringLiteral("case_sensitive")).toBool(false);
		question.trim = obj.value(QStringLiteral("trim")).toBool(true);

		question.answer =
		    normalizeAnswer(question.type, valueToStringList(obj.value(QStringLiteral("answer"))),
		                    question.caseSensitive);
		question.alternatives = normalizeAnswer(
		    question.type, valueToStringList(obj.value(QStringLiteral("alternatives"))),
		    question.caseSensitive);

		if (question.answer.isEmpty()) {
			if (errorOut)
				*errorOut = QObject::tr("第 %1 题在 key.json 中有答案，但无法识别。").arg(question.id);

			return {};
		}

		questions.append(question);
	}

	if (questions.isEmpty() && errorOut)
		*errorOut = QObject::tr("key.json 没有题目");

	return questions;
}

auto ChoiceJudge::parseKeyText(const QByteArray &json, QString *errorOut) -> QList<Question> {
	QJsonParseError parseError;
	const QJsonDocument document = QJsonDocument::fromJson(json, &parseError);

	if (parseError.error != QJsonParseError::NoError || ! document.isObject()) {
		if (errorOut)
			*errorOut = QObject::tr("无法解析 key.json: %1").arg(parseError.errorString());

		return {};
	}

	return parseKey(document.object(), errorOut);
}

auto ChoiceJudge::totalScoreOfFile(const QJsonObject &root) -> double {
	double total = 0.0;
	const double defaultScore = root.value(QStringLiteral("default_score")).toDouble(0.0);
	const QJsonArray array = root.value(QStringLiteral("questions")).toArray();

	for (const QJsonValue &one : array) {
		if (! one.isObject())
			continue;

		total += one.toObject().value(QStringLiteral("score")).toDouble(defaultScore);
	}

	return total;
}

auto ChoiceJudge::parseStudentAnswer(const QString &text) -> QMap<int, QStringList> {
	QMap<int, QStringList> out;

	// 与网页端 serializeAnswers() 对齐："<题号>: <答案>"，一题一行。
	// 题号分隔符还兼容 . - 、 ： 这些手写习惯。
	static const QRegularExpression numbered(QStringLiteral("^\\s*(\\d+)\\s*[:.\\-、：]\\s*(.*)$"));
	static const QRegularExpression separator(QStringLiteral("[\\s,，;；|/]+"));

	const QStringList lines = text.split(QRegularExpression(QStringLiteral("\\r?\\n")));

	for (const QString &line : lines) {
		if (line.trimmed().isEmpty())
			continue;

		const QRegularExpressionMatch match = numbered.match(line);

		QString idText;
		QString body;

		if (match.hasMatch()) {
			idText = match.captured(1);
			body = match.captured(2);
		} else {
			// 没有题号前缀时退化为「按出现顺序」，用 0 占位交给调用方补齐
			idText = QStringLiteral("0");
			body = line;
		}

		bool ok = false;
		const int id = idText.toInt(&ok);

		if (! ok)
			continue;

		const QStringList parts = body.split(separator, Qt::SkipEmptyParts);

		out.insert(id, parts.isEmpty() ? QStringList{body.trimmed()} : parts);
	}

	return out;
}

auto ChoiceJudge::judge(const QList<Question> &key, const QMap<int, QStringList> &student)
    -> JudgeReport {
	JudgeReport report;
	report.total = static_cast<int>(key.size());

	for (const Question &question : key) {
		QuestionResult item;
		item.id = question.id;
		item.type = question.type;
		item.fullScore = question.score;

		const QStringList rawStudent = student.value(question.id);
		item.student = normalizeAnswer(question.type, rawStudent, question.caseSensitive);
		item.expected = question.answer;
		item.answered = ! rawStudent.isEmpty() && ! rawStudent.join(QString()).trimmed().isEmpty();

		report.fullScore += question.score;

		if (item.answered)
			++report.answered;

		if (item.answered) {
			if (question.type == QuestionType::Multiple) {
				// 多选：把学生答案拼成一个字母集合
				const QString studentLetters = lettersOf(rawStudent.join(QString()));
				const QString expectedLetters = question.answer.value(0);

				QSet<QString> studentSet;
				QSet<QString> expectedSet;

				for (const QChar ch : studentLetters)
					studentSet.insert(QString(ch));

				for (const QChar ch : expectedLetters)
					expectedSet.insert(QString(ch));

				if (studentSet == expectedSet) {
					item.correct = true;
					item.score = question.score;
					++report.correct;
				} else if (question.partial && ! studentSet.isEmpty() &&
				           expectedSet.contains(studentSet)) {
					// 全部选项都对、只是漏选：按命中比例给分，这就是「半分制」的来源
					item.partialCredit = true;
					item.score = question.score * static_cast<double>(studentSet.size()) /
					             static_cast<double>(expectedSet.size());
				}
			} else if (question.type == QuestionType::Fill) {
				const QStringList accepted = question.answer + question.alternatives;

				for (const QString &one : item.student) {
					if (accepted.contains(one)) {
						item.correct = true;
						break;
					}
				}

				if (item.correct) {
					item.score = question.score;
					++report.correct;
				}
			} else {
				// 单选 / 判断
				const QString studentLetters = lettersOf(rawStudent.join(QString()));

				if (! studentLetters.isEmpty() &&
				    studentLetters == question.answer.value(0)) {
					item.correct = true;
					item.score = question.score;
					++report.correct;
				}
			}
		}

		report.score += item.score;
		report.items.append(item);
	}

	return report;
}

auto ChoiceJudge::formatReport(const JudgeReport &report) -> QString {
	QStringList lines;

	lines.append(QObject::tr("总得分: %1 / %2  答对 %3 / %4 题")
	                 .arg(numberToText(report.score), numberToText(report.fullScore))
	                 .arg(report.correct)
	                 .arg(report.total));

	for (const QuestionResult &item : report.items) {
		QString mark;

		if (! item.answered)
			mark = QObject::tr("(未作答)");
		else if (item.correct)
			mark = QObject::tr("✓ 正确");
		else if (item.partialCredit)
			mark = QObject::tr("◳ 部分");
		else
			mark = QObject::tr("✗ 错误");

		const QString student = item.student.isEmpty() ? QObject::tr("(未作答)")
		                                               : item.student.join(QStringLiteral(","));

		lines.append(QObject::tr("第 %1 题  你: %2   标答: %3   %4  %5/%6")
		                 .arg(item.id)
		                 .arg(student)
		                 .arg(item.expected.join(QStringLiteral(",")))
		                 .arg(mark)
		                 .arg(numberToText(item.score))
		                 .arg(numberToText(item.fullScore)));
	}

	return lines.join(QChar('\n'));
}
