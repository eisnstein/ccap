#ifndef CCAP_CCAP_H_
#define CCAP_CCAP_H_

#include <iostream>
#include <optional>
#include <string>
#include <vector>

namespace ccap {

//
// -- Definitions
//

enum class TerminationType { Exit, Exception };

class Argument {
 public:
  Argument() = delete;
  explicit Argument(const std::string &name);
  static auto WithName(const std::string &name) -> Argument;

  auto GetName() const -> const std::string &;
  auto GetShort() const -> char;
  auto SetShort(char s) -> Argument &;
  auto GetLong() const -> const std::string &;
  auto SetLong(const std::string &l) -> Argument &;
  auto GetValue() const -> std::optional<std::string>;
  auto SetValue(const std::string &value) -> Argument &;
  auto ExpectsValue() -> Argument &;
  auto IsExpectingValue() const -> bool;
  auto Required() -> Argument &;
  auto IsRequired() const -> bool;

  auto IsOption() const -> bool;
  auto IsGiven() const -> bool;
  auto SetGiven(bool value);

 private:
  std::string name_;
  std::string long_;
  char short_ = 0;
  std::string value_;

  bool required_ = false;
  bool expects_value_ = false;
  bool is_option_ = true;
  bool is_given_ = false;
};

class Args {
 public:
  Args() = delete;
  explicit Args(int argc, char const *argv[]);
  ~Args(){};

  static auto From(int argc, char const *argv[]) -> Args;

  // Adds an argument
  auto Arg(Argument item) -> Args &;

  // Gets the value of an argument
  auto Get(const std::string &arg_name) const -> std::optional<std::string>;

  auto IsGiven(const std::string &arg_name) const -> bool;

  // Parses and validates the given arguments
  // and updates the argument definitions.
  auto Parse() -> Args &;

  // Sets the prefered termination type
  auto SetTerminationType(TerminationType type) -> void;

  auto SetAbout(const std::string &version) -> Args &;
  auto SetAuthor(const std::string &author) -> Args &;
  auto SetName(const std::string &name) -> Args &;
  auto SetVersion(const std::string &version) -> Args &;

  auto ShowHelp() -> void;

  auto Terminate(const Argument &arg) -> void;

 private:
  std::vector<std::string> raw_args_;
  std::vector<Argument> args_;
  TerminationType terminateBy_ = TerminationType::Exit;
  int num_args_;
  std::string about_;
  std::string author_;
  std::string name_;
  std::string version_ = "0.0.1";

  auto ReadLongArg(int raw_arg_index) -> void;
  auto ReadShortArg(int raw_arg_index) -> void;
};

//
// -- Declarations
//

Argument::Argument(const std::string &name) : name_{name} {};

auto Argument::WithName(const std::string &name) -> Argument {
  return Argument(name);
}

auto Argument::GetName() const -> const std::string & { return name_; }

auto Argument::GetShort() const -> char { return short_; }

auto Argument::SetShort(char s) -> Argument & {
  short_ = s;
  return *this;
}

auto Argument::GetLong() const -> const std::string & { return long_; }

auto Argument::SetLong(const std::string &l) -> Argument & {
  long_ = l;
  return *this;
}

auto Argument::SetValue(const std::string &value) -> Argument & {
  value_ = value;
  return *this;
}

auto Argument::GetValue() const -> std::optional<std::string> {
  if (value_.size() > 0) {
    return value_;
  }

  return std::nullopt;
}

auto Argument::ExpectsValue() -> Argument & {
  expects_value_ = true;

  // If the arguments expects a value,
  // it cannot be an optional flag.
  is_option_ = false;

  return *this;
}

auto Argument::IsExpectingValue() const -> bool { return expects_value_; }

auto Argument::Required() -> Argument & {
  required_ = true;
  return *this;
}

auto Argument::IsRequired() const -> bool { return required_; }

auto Argument::IsOption() const -> bool { return is_option_; }

auto Argument::IsGiven() const -> bool { return is_given_; }

auto Argument::SetGiven(bool value) { is_given_ = value; }

// Initialize the class with the raw arguments.
Args::Args(int argc, char const *argv[]) {
  // Start at 1 because we don't need the program name
  for (int i = 1; i < argc; ++i) {
    raw_args_.push_back(argv[i]);
  }

  num_args_ = argc == 0 ? argc : argc - 1;
}

// Create an Args instance with the raw arguments.
auto Args::From(int argc, char const *argv[]) -> Args {
  return Args(argc, argv);
}

// Add an argument instance.
auto Args::Arg(Argument item) -> Args & {
  args_.push_back(item);
  return *this;
}

// Get the value of an argument.
auto Args::Get(const std::string &arg_name) const
    -> std::optional<std::string> {
  for (const auto &argument : args_) {
    if (argument.GetName() == arg_name) {
      return argument.GetValue();
    }
  }

  return std::nullopt;
}

// Check if the option arg is given.
auto Args::IsGiven(const std::string &arg_name) const -> bool {
  for (const auto &argument : args_) {
    if (argument.GetName() == arg_name) {
      return argument.IsOption() && argument.IsGiven();
    }
  }

  return false;
}

// Parse the raw arguments and set the according argument objects.
auto Args::Parse() -> Args & {
  for (int i = 0; i < num_args_; ++i) {
    if (raw_args_[i].starts_with("--")) {
      ReadLongArg(i);
    } else if (raw_args_[i].starts_with("-")) {
      ReadShortArg(i);
    } else {
      // args_[i].SetValue(raw_args_[i]);
    }
  }

  // After parsing the raw arguments we need to check that
  // all args that are marked as 'required' do have values.
  for (auto const &arg : args_) {
    if (arg.IsRequired() && !arg.GetValue().has_value()) {
      Terminate(arg);
    }
  }

  return *this;
}

auto Args::ReadLongArg(int raw_arg_index) -> void {
  // To get the name we "cut" away the two '-'
  // e.g.: --name => name
  std::string name = raw_args_[raw_arg_index].substr(2);
  if (name.length() == 0) {
    return;
  }

  if (name == "help") {
    ShowHelp();
  }

  // Find the arg with the given name
  for (auto &arg : args_) {
    if (arg.GetLong() == name) {
      // If the arg expects an value we take the value after
      // the arg name as that value.
      if (arg.IsExpectingValue()) {
        std::string v = raw_args_[raw_arg_index + 1];
        arg.SetValue(v);
      }

      // If the arg is an option we
      // mark the arg to be given.
      if (arg.IsOption()) {
        arg.SetGiven(true);
      }
    }
  }
}

auto Args::ReadShortArg(int raw_arg_index) -> void {
  char shortName = raw_args_[raw_arg_index][1];
  // Retrun when after the '-' nothing is provided
  if (shortName == 0) {
    return;
  }

  if (shortName == 'h') {
    ShowHelp();
  }

  for (auto &arg : args_) {
    if (arg.GetShort() == shortName) {
      if (arg.IsExpectingValue()) {
        // Return when no value is provided
        if (raw_args_.size() <= raw_arg_index + 1) {
          return;
        }

        std::string v = raw_args_[raw_arg_index + 1];
        arg.SetValue(v);
      }

      if (arg.IsOption()) {
        arg.SetGiven(true);
      }
    }
  }
}

auto Args::SetTerminationType(TerminationType type) -> void {
  terminateBy_ = type;
}

auto Args::SetAbout(const std::string &about) -> Args & {
  about_ = about;
  return *this;
}

auto Args::SetAuthor(const std::string &author) -> Args & {
  author_ = author;
  return *this;
}

auto Args::SetName(const std::string &name) -> Args & {
  name_ = name;
  return *this;
}

auto Args::SetVersion(const std::string &version) -> Args & {
  version_ = version;
  return *this;
}

auto Args::ShowHelp() -> void {
  std::cout << "TODO: Help Message\n";
  exit(0);
}

// Terminate the program based on the choosen termination type.
auto Args::Terminate(const Argument &arg) -> void {
  switch (terminateBy_) {
    case TerminationType::Exit:
      std::cerr << "Error: Missing required value for argument '"
                << arg.GetName() << "'\n";
      exit(EXIT_FAILURE);

    case TerminationType::Exception:
      throw "Exception";
  }
}

}  // namespace ccap

#endif  // CCAP_CCAP_H