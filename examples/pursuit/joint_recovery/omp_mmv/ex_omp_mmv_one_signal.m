close all;
clear all;
clc;
rng('default');
png_export = true;
pdf_export = false;
% Create the directory for storing images
[status_code,message,message_id] = mkdir('bin');

mf = SPX_Figures();

% Signal space 
N = 256;
% Number of measurements
M = 64;
% Sparsity level
K = 8;
% Number of signals
S = 1;
% Construct the signal generator.
gen  = SPX_SparseSignalGenerator(N, K, S);
% Generate bi-uniform signals
X = gen.biUniform(1, 2);
% Sensing matrix
Phi = SPX_SimpleDicts.gaussian_dict(M, N);
% Measurement vectors
Y = Phi.apply(X);
% OMP MMV solver instance
solver = SPX_OMP_MMV(Phi, K);
% Solve the sparse recovery problem
result = solver.solve(Y);
% Solution vector
Z = result.Z;

mf.new_figure('OMP MMV solution');
subplot(411);
stem(X, '.');
title('Sparse vector');
subplot(412);
stem(Z, '.');
title('Recovered sparse vector');
subplot(413);
stem(abs(X - Z), '.');
title('Recovery error');
subplot(414);
stem(Y, '.');
title('Measurement vector');