#pragma once

#include "types.hh"
#include "hash.hh"
#include "store-api.hh"

#include <map>
#include <variant>


namespace nix {


/* Abstract syntax of derivations. */

/// Pair of a hash, and how the file system was ingested
struct DerivationOutputHash {
    FileIngestionMethod method;
    Hash hash;
    std::string printMethodAlgo() const;
};

struct DerivationOutput
{
    StorePath path;
    std::optional<DerivationOutputHash> hash; /* hash used for expected hash computation */
    void parseHashInfo(FileIngestionMethod & recursive, Hash & hash) const;
};

typedef std::map<string, DerivationOutput> DerivationOutputs;

/* For inputs that are sub-derivations, we specify exactly which
   output IDs we are interested in. */
typedef std::map<StorePath, StringSet> DerivationInputs;

typedef std::map<string, string> StringPairs;

struct BasicDerivation
{
    DerivationOutputs outputs; /* keyed on symbolic IDs */
    StorePathSet inputSrcs; /* inputs that are sources */
    string platform;
    Path builder;
    Strings args;
    StringPairs env;

    BasicDerivation() { }
    virtual ~BasicDerivation() { };

    /* Return the path corresponding to the output identifier `id' in
       the given derivation. */
    const StorePath & findOutput(const std::string & id) const;

    bool isBuiltin() const;

    /* Return true iff this is a fixed-output derivation. */
    bool isFixedOutput() const;

    /* Return the output paths of a derivation. */
    StorePathSet outputPaths() const;

    /* Return the output names of a derivation. */
    StringSet outputNames() const;
};

struct Derivation : BasicDerivation
{
    DerivationInputs inputDrvs; /* inputs that are sub-derivations */

    /* Print a derivation. */
    std::string unparse(const Store & store, bool maskOutputs,
        std::map<std::string, StringSet> * actualInputs = nullptr) const;

    Derivation() { }
};


class Store;


/* Write a derivation to the Nix store, and return its path. */
StorePath writeDerivation(ref<Store> store,
    const Derivation & drv, std::string_view name, RepairFlag repair = NoRepair);

/* Read a derivation from a file. */
Derivation readDerivation(const Store & store, const Path & drvPath);

// FIXME: remove
bool isDerivation(const string & fileName);

// known CA drv's output hashes, current just for fixed-output derivations
// whose output hashes are always known since they are fixed up-front.
typedef std::map<std::string, Hash> CaOutputHashes;

typedef std::variant<
    Hash, // regular DRV normalized hash
    CaOutputHashes
> DrvHashModulo;

/* Returns hashes with the details of fixed-output subderivations
   expunged.

   A fixed-output derivation is a derivation whose outputs have a
   specified content hash and hash algorithm. (Currently they must have
   exactly one output (`out'), which is specified using the `outputHash'
   and `outputHashAlgo' attributes, but the algorithm doesn't assume
   this.) We don't want changes to such derivations to propagate upwards
   through the dependency graph, changing output paths everywhere.

   For instance, if we change the url in a call to the `fetchurl'
   function, we do not want to rebuild everything depending on it---after
   all, (the hash of) the file being downloaded is unchanged.  So the
   *output paths* should not change. On the other hand, the *derivation
   paths* should change to reflect the new dependency graph.

   For fixed-output derivations, this returns a map from the name of
   each output to its hash, unique up to the output's contents.

   For regular derivations, it returns a single hash of the derivation
   ATerm, after subderivations have been likewise expunged from that
   derivation.
 */
DrvHashModulo hashDerivationModulo(Store & store, const Derivation & drv, bool maskOutputs);

/* Memoisation of hashDerivationModulo(). */
typedef std::map<StorePath, DrvHashModulo> DrvHashes;

extern DrvHashes drvHashes; // FIXME: global, not thread-safe

bool wantOutput(const string & output, const std::set<string> & wanted);

struct Source;
struct Sink;

Source & readDerivation(Source & in, const Store & store, BasicDerivation & drv);
void writeDerivation(Sink & out, const Store & store, const BasicDerivation & drv);

std::string hashPlaceholder(const std::string & outputName);

}
