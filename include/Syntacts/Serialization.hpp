#pragma once

#include <Syntacts/Config.hpp>
#include <memory>
#include <string>

//=============================================================================
// SERIALIZATION MACROS FOR CEREAL
//=============================================================================

// We forward declare cereal::access so end-users don't need to include cereal
namespace cereal {
    class access;
}

#define SERIALIZE(...) friend class cereal::access; \
                       template <class Archive> \
                       void serialize(Archive& archive) { \
                           archive(__VA_ARGS__); \
                       }
#define PARENT(T) ::cereal::make_nvp(#T, cereal::base_class<T>(this))
#define MEMBER(T) ::cereal::make_nvp(#T, T)

//============================================================================
// SAVE/LOAD FUNCTIONS
//============================================================================

namespace tact {

class Cue;

enum class SerialFormat {
    Binary,
    JSON
};

/// Returns the directory to which all library cues are saved/loaded
const std::string& getLibraryDirectory();

/// Saves a cue to the global Syntacts cue library
bool save(const std::shared_ptr<Cue>& cue, const std::string& name, SerialFormat format = SerialFormat::Binary);

/// Loads a Cue from the global Syntacts Cue library
bool load(std::shared_ptr<Cue>& cue, const std::string& name, SerialFormat format = SerialFormat::Binary);

}