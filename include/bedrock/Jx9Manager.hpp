/*
 * (C) 2021 The University of Chicago
 *
 * See COPYRIGHT in top-level directory.
 */
#ifndef __BEDROCK_JX9_MANAGER_HPP
#define __BEDROCK_JX9_MANAGER_HPP

#include <string>
#include <vector>
#include <unordered_map>
#include <memory>
#include <bedrock/MPIEnv.hpp>

namespace bedrock {

class Server;
class ServerImpl;
class Jx9ManagerImpl;
class ProviderManager;

/**
 * @brief The Jx9Manager class encapsulates a Jx9 engine.
 */
class Jx9Manager {

    friend class Server;
    friend class ServerImpl;
    friend class ProviderManager;

  public:
    /**
     * @brief Constructor.
     */
    Jx9Manager(MPIEnv mpiEnv);

    /**
     * @brief Copy-constructor.
     */
    Jx9Manager(const Jx9Manager&);

    /**
     * @brief Move-constructor.
     */
    Jx9Manager(Jx9Manager&&);

    /**
     * @brief Copy-assignment operator.
     */
    Jx9Manager& operator=(const Jx9Manager&);

    /**
     * @brief Move-assignment operator.
     */
    Jx9Manager& operator=(Jx9Manager&&);

    /**
     * @brief Destructor.
     */
    ~Jx9Manager();

    /**
     * @brief Checks whether the Jx9Manager instance is valid.
     */
    operator bool() const;

    /**
     * @brief Install a global variable that will be made available
     * whenever a query is made.
     *
     * @param name Name of the variable.
     * @param value Value (JSON formatted string).
     */
    void setVariable(const std::string& name,
                     const std::string& value);

    /**
     * @brief Unset a global variable.
     */
    void unsetVariable(const std::string& name);

    /**
     * @brief This function executes the provided script
     * and returns the content of value returned by the script.
     *
     * In case of a Jx9 error, this function will throw
     * an Exception.
     *
     * @param script Content of the script to execute.
     * @param variables variables to add to the jx9 VM.
     *
     * @return The serialized returned value of the script.
     */
    std::string executeQuery(
        const std::string&                                  script,
        const std::unordered_map<std::string, std::string>& variables) const;

    /**
     * @brief Evaluate a condition written in Jx9.
     */
    bool evaluateCondition(
        const std::string&                                  condition,
        const std::unordered_map<std::string, std::string>& variables) const;

  private:
    std::shared_ptr<Jx9ManagerImpl> self;

    inline operator std::shared_ptr<Jx9ManagerImpl>() const { return self; }

    inline Jx9Manager(std::shared_ptr<Jx9ManagerImpl> impl)
    : self(std::move(impl)) {}
};

} // namespace bedrock

#endif
