/*
 * SPDX-FileCopyrightText: 2026 Project LemonLime Online
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#pragma once
//

#include <QJsonObject>
#include <QList>
#include <QMap>
#include <QString>
#include <QStringList>

// LemonNext 选择题（Choice）——教师维护题面 paper.md 与答案 key.json，
// 选手在网页上作答，服务端把作答存成一行一题的纯文本，判分时与 key.json 比对。
//
// key.json 结构（自 LemonNext 1.0.0 二进制还原）：
// {
//   "version": 1,
//   "default_score": 3,
//   "questions": [
//     {
//       "id": 1,
//       "type": "single" | "multiple" | "multi" | "truefalse" | "tf" | "judge" | "fill",
//       "score": 3,
//       "scoring": "partial",
//       "case_sensitive": false,
//       "trim": true,
//       "answer": "A" | ["A", "B"] | "3.14",
//       "alternatives": ["3.1416"]
//     }
//   ]
// }
//
// 学生答案文件（一行一题，题号对应 key.json 的 id）：
//   1: A
//   2: AB
//   3: 3.14
class ChoiceJudge {
  public:
    enum class QuestionType { Single, Multiple, TrueFalse, Fill };

    struct Question {
        int id = 0;
        QuestionType type = QuestionType::Single;
        double score = 0.0;
        QStringList answer;       // 标准答案（选择题为大写字母，填空为字符串）
        QStringList alternatives; // 填空可接受的等价写法
        bool partial = false;     // scoring == "partial"：多选/填空允许部分给分
        bool caseSensitive = false;
        bool trim = true;
    };

    struct QuestionResult {
        int id = 0;
        QuestionType type = QuestionType::Single;
        bool answered = false;
        bool correct = false;
        bool partialCredit = false;
        double score = 0.0;
        double fullScore = 0.0;
        QStringList student;
        QStringList expected;
    };

    struct JudgeReport {
        double score = 0.0;
        double fullScore = 0.0;
        int answered = 0;
        int total = 0;
        int correct = 0;
        QList<QuestionResult> items;
    };

    // ---- key.json ----
    static auto parseKey(const QJsonObject &root, QString *errorOut = nullptr) -> QList<Question>;
    static auto parseKeyText(const QByteArray &json, QString *errorOut = nullptr) -> QList<Question>;
    // 满分取自 key.json 里所有题分数之和
    static auto totalScoreOfFile(const QJsonObject &root) -> double;

    // ---- 学生答案 ----
    // 接受 "1: A" / "1. A" / "1-A" / "1 A" 等写法，一题一行
    static auto parseStudentAnswer(const QString &text) -> QMap<int, QStringList>;

    // ---- 判分 ----
    static auto judge(const QList<Question> &key, const QMap<int, QStringList> &student) -> JudgeReport;
    static auto formatReport(const JudgeReport &report) -> QString;

    static auto typeName(QuestionType type) -> QString;
    static auto parseTypeName(const QString &raw) -> QuestionType;
    static auto normalizeAnswer(QuestionType type, const QStringList &raw, bool caseSensitive) -> QStringList;
};
