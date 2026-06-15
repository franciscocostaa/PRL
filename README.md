[![✗](https://img.shields.io/badge/Release-v2.0.0-ffb600.svg?style=for-the-badge)](https://github.com/franciscocostaa/PRL/releases)

[![✗](https://github.com/franciscocostaa/PRL/actions/workflows/pipeline.yaml/badge.svg?branch=production)](https://github.com/franciscocostaa/PRL/actions/workflows/pipeline.yaml)

# PRL — Pricing Rule Language

A domain-specific language (DSL) for declarative definition of pricing rules, discounts, surcharges, and commercial invariants. Built with Flex and Bison on top of the Flex-Bison-Compiler base framework.

* [Language Overview](#language-overview)
* [Postponed Features](#postponed-features)
* [Requirements](#requirements)
* [Configuration](#configuration)
* [Commands](#commands)
* [CI/CD](#cicd)
* [Recommended Extensions](#recommended-extensions)

## Language Overview

A PRL program is composed of one or more **campaigns**, optional **simulation** blocks (constraint-satisfaction questions over a model), and optional **export** statements.

```prl
campaign PricingLatam {
    entities:
        client: Client
        product: Product
        order: Order

    invariants:
        assert order.total_discount <= 40%

    rules:
        rule "Mayorista_B2B" priority 50 {
            if client.segment == "B2B" and order.item_count >= 100
            then discount(25%)
        }
        rule "Bloqueo_Fraude" priority 999 {
            if client.loyalty_points == 0 and order.total_amount > 5000
            then reject("Operación excede monto seguro para usuario nuevo")
        }
}

export PricingLatam to json
```

### Supported actions

| Action | Description |
| :----- | :---------- |
| `discount(<value>%)` | Apply a percentage discount |
| `discount_fixed(<value>)` | Apply a fixed-amount discount |
| `surcharge(<value>%)` | Apply a percentage surcharge |
| `reject("<reason>")` | Reject the transaction with a message |

### Supported operators

| Type | Operators |
| :--- | :-------- |
| Relational | `==`, `!=`, `>`, `<`, `>=`, `<=` |
| Logical | `and`, `or`, `not` |
| Membership | `in [...]` |
| Arithmetic | `+`, `-`, `*`, `/` |

### Entity schemas

Entities may declare a typed property schema. When present, field accesses are validated and
statically typed against it (instead of being treated as dynamic):

```prl
client: Client { segment: string, tier: integer }
```

### Simulation (satisfiability)

A `simulation` poses a constraint-satisfaction question over a model: given a set of
constraints, does a solution exist that *also* meets every expected constraint? `satisfiable`
/ `unsatisfiable` states the expected answer.

```prl
simulation "Existe combo valido" on PricingLatam {
    given:
        client.segment == "B2B"
        order.item_count >= 100
    expect:
        order.total_amount > 0
        satisfiable
}
```

## Backend (Stage III)

Stage III adds the compiler backend on top of the frontend AST:

* **Semantic analysis** (`backend/semantic-analysis/`): a symbol table organised as a
  stack of scopes (global → campaign) validates the program — unique campaign / entity /
  rule / property names, entity-binding of `entity.field` references (enforced when a campaign
  declares an `entities:` section), **schema-aware type checking** (a field declared in an
  entity schema is typed by its declared type instead of being dynamic), **arithmetic
  type checking**, action type checking (`discount`/`surcharge` require a percentage,
  `discount_fixed` an integer), and that every `export` and `simulation` targets a declared
  campaign. Invalid programs are rejected with a non-zero exit code.
* **Code generation** (`backend/code-generation/`): each exported campaign is serialised to
  **JSON** on standard output, and each `simulation` is materialised as a
  constraint-satisfaction problem (`model`, `given`, `expect`, `question`).

## Postponed Features

The following constructs remain **future work** (documented in the final report under
"Futuras Extensiones"); they are not part of the fundamental Stage III deliverable:

### Simulation solver

`simulation` blocks are fully parsed, type-checked against the model schema, and emitted as a
constraint-satisfaction problem (see *Simulation (satisfiability)* above). What remains future
work is the **solver** that actually searches for a satisfying assignment — or proves
`unsatisfiable` — typically by translating the constraints to an SMT solver such as Z3.

### Campaign inheritance

```prl
extend CyberMondayLatam from PricingLatam {
    drop rule "Mayorista_B2B"

    rules:
        rule "Cyber_Descuento_Global" priority 10 {
            if order.item_count > 0 then discount(30%)
        }
}
```

`extend ... from ...` derives a new campaign from a base one. `drop rule "<name>"` removes an inherited rule.

### Static analysis

```prl
detect_shadowing on CyberMondayLatam
```

`detect_shadowing` warns about rules that can never be evaluated because a higher-priority rule with a broader condition already covers them. Requires rule-condition analysis.

## Requirements

* [Docker v28.3.2](https://www.docker.com/)

## Configuration

Set the following environment variables to control and configure the behaviour of the application:

| Name                  | Default | Description                                                                                                                                                           |
| :-------------------- | :-----: | :-------------------------------------------------------------------------------------------------------------------------------------------------------------------- |
| `ENVIRONMENT`         | `Local` | The active environment name. The available environments are: `Local`, `Development` and `Production`.                                                                 |
| `LOG_IGNORED_LEXEMES` | `true`  | When `true`, logs all of the ignored lexemes found with Flex at `DEBUGGING` level. To remove those logs from the console output set it to `false`.                    |
| `LOGGING_LEVEL`       | `ALL`   | The minimum level to log in the console output. From lower to higher, the available levels are: `ALL`, `DEBUGGING`, `INFORMATION`, `WARNING`, `ERROR` and `CRITICAL`. |

_Docker Compose_ can read the variables from an `.env` file too (see `compose.yaml` file).

## Commands

### Start

Rises an ephemeral container, ready to start development:

```bash
docker compose run --rm compiler
```

### Build

Builds or rebuilds the entire compiler:

```bash
src/main/bash/build.sh
```

### Run

Compiles a program:

```bash
src/main/bash/run.sh <program>
```

where `<program>` is the path to the file that represents its entry-point.

### Test

Executes every available unit-test under `src/test/c` folder:

```bash
src/main/bash/test.sh
```

### Stop

Logout, destroy the ephemeral containers and shutdowns the cluster:

```bash
exit
docker compose down
```

### Docker

| Command                                 | Description                                             |
| :-------------------------------------- | :------------------------------------------------------ |
| `docker builder prune --all`            | Removes all builds and complete build cache.            |
| `docker compose --progress=plain build` | Forces a build or rebuild of the images in the cluster. |
| `docker image prune`                    | Removes all of the dangling images from Docker.         |
| `docker network prune`                  | Removes unused networks from Docker.                    |
| `docker volume prune`                   | Removes unused volumes from Docker.                     |

## CI/CD

To trigger an automatic integration on every push or PR (_Pull Request_), you must activate _GitHub Actions_ in the _Settings_ tab. Use the following configuration:

| Key                                                        | Value                                               |
| :--------------------------------------------------------- | :-------------------------------------------------- |
| `Actions permissions`                                      | `Allow all actions and reusable workflows`          |
| `Allow GitHub Actions to create and approve pull requests` | `false`                                             |
| `Artifact and log retention`                               | `30 days`                                           |
| `Fork pull request workflows from outside collaborators`   | `Require approval for all outside collaborators`    |
| `Workflow permissions`                                     | `Read repository contents and packages permissions` |

## Recommended Extensions

* [C/C++](https://marketplace.visualstudio.com/items?itemName=ms-vscode.cpptools)
* [CMake Tools](https://marketplace.visualstudio.com/items?itemName=ms-vscode.cmake-tools)
* [Yash](https://marketplace.visualstudio.com/items?itemName=daohong-emilio.yash)
