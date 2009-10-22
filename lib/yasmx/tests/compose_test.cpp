//
//  Copyright (C) 2007  Peter Johnson
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions
// are met:
// 1. Redistributions of source code must retain the above copyright
//    notice, this list of conditions and the following disclaimer.
// 2. Redistributions in binary form must reproduce the above copyright
//    notice, this list of conditions and the following disclaimer in the
//    documentation and/or other materials provided with the distribution.
//
// THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND OTHER CONTRIBUTORS ``AS IS''
// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
// ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR OTHER CONTRIBUTORS BE
// LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
// CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
// SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
// INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
// CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
// ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
// POSSIBILITY OF SUCH DAMAGE.
//
#include <gtest/gtest.h>

#include "llvm/Support/raw_ostream.h"
#include "yasmx/Support/Compose.h"

TEST(ComposeTest, ByClass)
{
    std::string out;

    // demonstrate basic usage
    out = String::Composer("There are %1 cows in them %2.") % 15 % "fields";
    EXPECT_EQ("There are 15 cows in them fields.", out);

    using String::Composer;

    // demonstrate argument repetition
    out = Composer("To %1, or not to %1... is actually not a question.") % "be";
    EXPECT_EQ("To be, or not to be... is actually not a question.", out);

    // demonstrate leaving out arguments
    out = Composer("Primetime: %2  %3  %5  %7") % 1 % 2 % 3 % 4 % 5 % 6 % 7;
    EXPECT_EQ("Primetime: 2  3  5  7", out);

    // demonstrate % escaping
    out = Composer("Using % before a %%1 causes the %1 to be escaped") % "%1";
    EXPECT_EQ("Using % before a %1 causes the %1 to be escaped", out);

    out = Composer("Four percent in a row: %%%%%%%%");
    EXPECT_EQ("Four percent in a row: %%%%", out);

    // test % escaping at the string ends
    out = Composer("%% This is like a LaTeX comment %%");
    EXPECT_EQ("% This is like a LaTeX comment %", out);

    // test % specs at the string ends
    out = Composer("%1 %2") % "Hello" % "World!";
    EXPECT_EQ("Hello World!", out);

    // test a bunch of arguments
    out = Composer("%1 %2 %3 %4 %5 %6, %7 %8!") % "May" % "the" % "Force" %
        "be" % "with" % "you" % "Woung" % "Skytalker";
    EXPECT_EQ("May the Force be with you, Woung Skytalker!", out);
}

TEST(ComposeTest, ByFunction)
{
    std::string out;

    // demonstrate basic usage
    out = String::Compose("There are %1 cows in them %2.", 15, "fields");
    EXPECT_EQ("There are 15 cows in them fields.", out);

    using String::Compose;

    // demonstrate argument repetition
    out = Compose("To %1, or not to %1... is actually not a question.", "be");
    EXPECT_EQ("To be, or not to be... is actually not a question.", out);

    // demonstrate leaving out arguments
    out = Compose("Primetime: %2  %3  %5  %7", 1, 2, 3, 4, 5, 6, 7);
    EXPECT_EQ("Primetime: 2  3  5  7", out);

    // demonstrate % escaping
    out = Compose("Using % before a %%1 causes the %1 to be escaped", "%1");
    EXPECT_EQ("Using % before a %1 causes the %1 to be escaped", out);

    out = Compose("Four percent in a row: %%%%%%%%");
    EXPECT_EQ("Four percent in a row: %%%%", out);

    // test % escaping at the string ends
    out = Compose("%% This is like a LaTeX comment %%");
    EXPECT_EQ("% This is like a LaTeX comment %", out);

    // test % specs at the string ends
    out = Compose("%1 %2", "Hello", "World!");
    EXPECT_EQ("Hello World!", out);

    // test a bunch of arguments
    out = Compose("%1 %2 %3 %4 %5 %6, %7 %8!", "May", "the", "Force",
                  "be", "with", "you", "Woung", "Skytalker");
    EXPECT_EQ("May the Force be with you, Woung Skytalker!", out);
}

TEST(ComposeTest, Empty)
{
    std::string out;

    out = String::Composer("1: %1 2: %2 3: %3") % "" % "b" % "c";
    EXPECT_EQ("1:  2: b 3: c", out);

    out = String::Composer("1: %1 2: %2 3: %3") % "a" % "" % "c";
    EXPECT_EQ("1: a 2:  3: c", out);

    out = String::Composer("1: %1 2: %2 3: %3") % "a" % "b" % "";
    EXPECT_EQ("1: a 2: b 3: ", out);

    out = String::Composer("1: %1 2: %2 3: %3 ") % "a" % "b" % "";
    EXPECT_EQ("1: a 2: b 3:  ", out);
}

static std::string
func(const std::string& arg)
{
    return arg;
}

TEST(ComposeTest, FuncOut)
{
    using String::Composer;
    EXPECT_EQ("composing in a function is fun!",
              func(Composer("composing in a %1 is fun!") % "function"));
}

TEST(ComposeTest, StreamOut)
{
    std::string s;
    llvm::raw_string_ostream os(s);
    os << "Here's some " << String::Composer("formatted %1 %2") % "text" % "for"
        << " you!";
    EXPECT_EQ("Here's some formatted text for you!", os.str());
}

TEST(ComposeTest, Copy)
{
    using String::Composer;

    Composer x("%1 %2");
    x.AutoArg("foo");

    Composer a = x, b = x;
    a.AutoArg("bar");
    b.AutoArg("baz");

    EXPECT_EQ("foo bar", a.getStr());
    EXPECT_EQ("foo baz", b.getStr());
}

TEST(ComposeTest, Partial)
{
    EXPECT_EQ("foo bar %3", String::Compose("%1 %2 %3", "foo", "bar"));
}
