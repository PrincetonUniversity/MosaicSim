int main(int argc, char** argv) {

  char *x_to_y_fname;
  bgraph x_to_y_bgraph;
  weight_type *y_project;
  
  assert(argc == 2);
  x_to_y_fname = argv[1];
  x_to_y_bgraph = parse_bgraph(x_to_y_fname);
  y_project = (weight_type*) calloc(get_projection_size(x_to_y_bgraph), sizeof(weight_type));
  //std::cout << sizeof(weight_type)*get_projection_size(x_to_y_bgraph) << std::endl;
  assert(y_project);
  
  preprocess(x_to_y_bgraph, y_project);
  
  auto start = std::chrono::system_clock::now();
  
  printf("Running kernel\n");
  _kernel_(x_to_y_bgraph, y_project, 0 , 1);
  
  auto end = std::chrono::system_clock::now();
  std::chrono::duration<double> elapsed_seconds = end-start;
  std::cout << "Elapsed time: " << elapsed_seconds.count() << "s\n";

  unsigned long long total = 0.0;
  for (unsigned long i = 0; i < get_projection_size(x_to_y_bgraph); i++) {
    total += 1; //((i%2048)*y_project[i]);
  }
  printf("Finished hash: %llu\n", total);

  clean_bgraph(x_to_y_bgraph);
  free(y_project);

  return 0;
}
