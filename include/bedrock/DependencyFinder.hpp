/*
 * (C) 2020 The University of Chicago
 *
 * See COPYRIGHT in top-level directory.
 */
#ifndef __BEDROCK_DEPENDENCY_RESOLVER_HPP
#define __BEDROCK_DEPENDENCY_RESOLVER_HPP

#include <bedrock/MargoContext.hpp>
#include <bedrock/ABTioContext.hpp>
#include <bedrock/ProviderManager.hpp>
#include <bedrock/VoidPtr.hpp>
#include <string>
#include <memory>

namespace bedrock {

class DependencyFinderImpl;

/**
 * @brief A DependencyFinder is an object that takes a dependency
 * specification and resolves it into a void* handle to that dependency.
 */
class DependencyFinder {

  public:
    /**
     * @brief Constructor.
     *
     * @param margo Margo context
     */
    DependencyFinder(const MargoContext& margo, const ABTioContext& abtio,
                     const ProviderManager& pmanager);

    /**
     * @brief Copy-constructor.
     */
    DependencyFinder(const DependencyFinder&);

    /**
     * @brief Move-constructor.
     */
    DependencyFinder(DependencyFinder&&);

    /**
     * @brief Copy-assignment operator.
     */
    DependencyFinder& operator=(const DependencyFinder&);

    /**
     * @brief Move-assignment operator.
     */
    DependencyFinder& operator=(DependencyFinder&&);

    /**
     * @brief Destructor.
     */
    ~DependencyFinder();

    /**
     * @brief Checks whether the DependencyFinder instance is valid.
     */
    operator bool() const;

    /**
     * @brief Resolve a specification, returning a void* handle to it.
     * This function throws an exception if the specification could not
     * be resolved. A specification string follows the following grammar:
     *
     * SPEC       := IDENTIFIER
     *            |  IDENTIFIER '@' LOCATION
     * IDENTIFIER := NAME
     *            |  TYPE ':' ID
     * LOCATION   := ADDRESS
     *            |  'ssg://' GROUP '/' RANK
     * ADDRESS    := <mercury address>
     * NAME       := <qualified identifier>
     * ID         := <provider id>
     *
     * @param type Type of dependency.
     * @param spec Specification string.
     *
     * @return handle to dependency
     */
    VoidPtr find(const std::string& type, const std::string& spec) const;

  private:
    std::shared_ptr<DependencyFinderImpl> self;

    inline operator std::shared_ptr<DependencyFinderImpl>() const {
        return self;
    }

    inline DependencyFinder(std::shared_ptr<DependencyFinderImpl> impl)
    : self(std::move(impl)) {}
};

} // namespace bedrock

#endif
