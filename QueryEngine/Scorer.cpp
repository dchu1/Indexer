#include "Scorer.h"
#include <cmath>

/// <summary>
/// Uses BM25 to score document
/// </summary>
/// <param name="args">[Doc Length, #docs with term1, # term1 in doc, #docs with term2, # term2 in doc...]</param>
/// <returns>score</returns>
double ScorerBM25::score(const std::vector<double>& args) const
{
	double accumulator = 0;
	double d = args[0];
	for (int i = 1; i < args.size(); i+=2 )
	{
		double ft = args[i];
		double fdt = args[i + 1];
		double x1 = log10((_N - ft + 0.5) / (ft + 0.5));
		double x2 = ((_k1 + 1) * fdt) / (_k1 * ((1 - _b) + _b * d / _davg));
		accumulator += x1 * x2;
		//accumulator += log10((_N - args[i] + 0.5) / (args[i] + 0.5)) * ((_k1 + 1) * args[i + 1]) / (_k1 * ((1 - _b) + _b * args[0] / _davg));
	}
	return accumulator;
}

// args = Doc Length, #docs with term1, # term1 in doc, #docs with term2, # term2 in doc... 
double ScorerCosMeasure::score(const std::vector<double>& args) const
{
	double accumulator = 0;
	for (int i = 1; i < args.size(); i += 2)
	{
		accumulator += log(1 + _N / args[i]) * (1 + log(args[i + 1])) / sqrt(_davg);
	}
	return accumulator;
}