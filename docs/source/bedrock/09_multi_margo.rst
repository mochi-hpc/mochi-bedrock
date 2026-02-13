Multiple Margo instances
========================

By default, a Bedrock daemon initializes a single Margo instance (and therefore
a single Mercury transport). In some cases, however, you may want a single process
to listen on multiple networks or protocols simultaneously, for instance shared
memory for intra-node communication and a fabric transport for inter-node
communication, or a high-performance network inside a cluster and TCP for Bedrock
to be reachable from outside. Bedrock supports this by allowing the :code:`margo`
field of the JSON configuration to be an **array** of Margo configurations instead
of a single object.

Configuring multiple engines
----------------------------

When the :code:`margo` field is an array, each element follows the same format
as a regular Margo configuration object. Bedrock will create one Margo instance
(engine) per element. The engines are numbered starting from 0 in the order
they appear in the array. Engine 0 is the **parent** of all subsequent engines.

.. literalinclude:: ../../examples/bedrock/09_multi_margo/multi_margo.json
   :language: json

In the example above, engine 0 listens on shared memory (:code:`na+sm`) and
engine 1 listens on TCP (:code:`ofi+tcp`). Engine 0 has its own pools,
execution streams, and progress/RPC pool configuration. Engine 1 (a child
instance) only specifies a Mercury address and pool/progress configuration,
referencing the engine 0's Argobots environment.

.. important::
   Child instances (engines with index > 0) share the Argobots environment
   of the parent (engine 0). Their configuration **must not** contain the
   following fields:

   - :code:`argobots` (pools and execution streams are inherited from the parent)
   - :code:`rpc_thread_count`
   - :code:`use_progress_thread`

   Only engine 0 should define Argobots pools and execution streams.
   Child engines reference pools from the parent when configuring
   :code:`progress_pool` and :code:`rpc_pool`.

.. note::
   When using multiple engines, the protocol for each engine is inferred from
   the :code:`mercury.address` field in that engine's configuration. The
   :code:`address` command-line argument to the :code:`bedrock` program becomes
   optional; if provided, it serves as a fallback for any engine whose
   configuration does not contain a :code:`mercury.address` field.

.. note::
   Bedrock automatically registers its management RPCs (configuration queries,
   pool/xstream management, etc.) on **all** engines, so a client can connect to
   any engine to query or reconfigure the service.

Assigning providers to engines
------------------------------

Each provider can be assigned to a specific engine using the :code:`engine` field
in its configuration. This integer field (defaulting to 0) specifies the index
of the engine on which the provider will be registered.

.. literalinclude:: ../../examples/bedrock/09_multi_margo/provider_with_engine.json
   :language: json

In this example, the provider is registered on engine 1 (the TCP engine from
the earlier configuration). All RPCs for this provider will be registered against
that engine (i.e., they can be received only by sending the RPC to that engine's
address).

.. important::
   When :code:`engine` is omitted, the provider defaults to engine 0.
   The engine index must be less than the total number of engines defined
   in the :code:`margo` array, otherwise Bedrock will report an error.

Depending on a Margo instance
------------------------------

Components can declare a dependency on a Margo instance itself. This is useful
for components that need to create their own RPCs or endpoints on a specific
network. A Margo dependency provides the component with a :code:`margo_instance_id`
that it can use directly.

To depend on a Margo instance, use the dependency type :code:`"margo"` and
reference the engine by its index:

.. literalinclude:: ../../examples/bedrock/09_multi_margo/margo_dependency.json
   :language: json

In this example, the provider is registered on engine 0, but also receives a
handle to engine 1's Margo instance through its :code:`secondary_margo`
dependency. The Bedropck component can retrieve it using:

.. code-block:: cpp

   #include <margo.h>

   static std::shared_ptr<bedrock::AbstractComponent>
   Register(const bedrock::ComponentArgs& args) {
       // Retrieve the margo_instance_id from the dependency
       auto secondary_mid = args.dependencies["secondary_margo"][0]
                                ->getHandle<margo_instance_id>();
       // Use secondary_mid to register RPCs, create endpoints, etc.
       ...
   }

When specifying a Margo dependency in the :code:`dependencies` section,
the value can be:

- An **integer**: the engine index (e.g. :code:`1`);
- A **string** containing an integer: the engine index as a string (e.g. :code:`"1"` ).

.. warning::

   The engine index must refer to a valid engine. Bedrock will raise an error
   if the index is out of range.


Output configuration
--------------------

When querying the configuration of a Bedrock daemon running with multiple engines,
the :code:`margo` field in the output will be a JSON array (one element per engine).
When running with a single engine, the :code:`margo` field remains a JSON object
for backward compatibility.

Providers in the output configuration will include an :code:`engine` field
indicating which engine they are registered on.
