# PILLS

This repository provides the complete theoretical proofs and implementation of PILLS, including its offline, online, and serving modules.

## Theoretical Proofs

Complete proofs of all propositions are available in `proofs.pdf`.



## Datasets & Experiments

All datasets and experimental materials are publicly available. **Please use the links below to download the datasets and heat configurations.**

- **Datasets:** Due to GitHub's file size limitations, the datasets are hosted externally. [Download Datasets](https://mega.nz/folder/raAkmRbR#jCE6MdG5307881KwDwROMg).
- **Heat Configurations:** Heat settings used in our experiments are available at [Download Heat Configurations](https://mega.nz/file/lStFhDRb#qHBZgidg6kblVMjodf0evC8bk0hduUkD0VGfR9eXSa0).
- **Public Dataset Experiments:** `public_datasets_experiments.zip` — experimental results on the three public datasets, included in this repository.

.

## Implementation

This repository provides the implementation of PILLS, organized into three modules: **offline**, **online**, and **serving**.

Moreover, we contacted the authors of **Rap** to request their source code but did not receive a response. Therefore, we provide our own implementation for comparison.

## Quick build & run

- Build the offline module:

```bash
cd PILLS-offline/src
make
# executable: ./main
```

- Build the online module:

```bash
cd PILLS-online/src
make
# executable: ./main
```

- Build the serving module:

```bash
cd PILLS-serving/src
make
# executable: ./main
```

Refer to the subdirectory `README.md` files for detailed usage and command-line arguments:

- `PILLS-offline/README.md`: input formats, output naming conventions, and usage examples.
- `PILLS-online/README.md`: online module arguments and brief descriptions of core components.
- `PILLS-serving/README.md`: how to run serving against offline results and the probability distribution strategy used for allocation.

